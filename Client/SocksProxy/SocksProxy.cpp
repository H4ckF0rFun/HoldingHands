#include "SocksProxy.h"
#include "IOCP.h"
#include "utils.h"
#include <stdint.h>
#include <assert.h>
#include <winioctl.h>

#include "SocksProxyUDP.h"
#include "SocksProxyTcp.h"

CSocksProxy::CSocksProxy(CClient *pClient):
CEventHandler(pClient,SOCKS_PROXY)
{
	memset(m_Clients, 0 , sizeof(m_Clients));
}

CSocksProxy::~CSocksProxy()
{
	dbg_log("CSocksProxy::~CSocksProxy()");
}

void CSocksProxy::OnOpen()
{
	
}

void CSocksProxy::OnClose()
{
	//Close all proxy connection.
	for (int i = 0; i < MAX_CLIENT_COUNT; i++)
	{
		if (!m_Clients[i])
			continue;

		OnProxyClose(i);
	}
}

/*
	field        size
---------------------------
	clientID	 4
	cmd			 1
	addrtype     1
	port         2
	addr         var
*/


//只有在本地和远程都发送关闭消息后才删除Ctx.
void CSocksProxy::OnProxyClose(UINT32 ClientID)
{
	assert(ClientID < MAX_CLIENT_COUNT);
	assert(m_Clients[ClientID]);											//????触发异常了.
	
	if (m_Clients[ClientID] != (CIOCPSocket*)-1)
	{
		m_Clients[ClientID]->Close();
		m_Clients[ClientID]->Put();
	}
	
	//dbg_log("Client[%d] Close Finished", ClientID);
	m_Clients[ClientID] = NULL;
}



void CSocksProxy::ConnectHandle(UINT32 ClientID, USHORT  Port, BYTE addrType, BYTE*addr)
{
	CSocksProxyTcp * proxy = new CSocksProxyTcp(this, ClientID);
	HOSTENT *pHost;
	SOCKADDR_IN ipv4_addr = { 0 };
	BOOL bAssociated = FALSE;
	UINT32 Error = 0;
	vec bufs[2];

	assert(m_Clients[ClientID] == NULL);

	m_Clients[ClientID] = proxy;

	if (!proxy->Create())
	{
		dbg_log("con->Create() failed");
		proxy->Close();
		goto _handle_failed;
	}

	if (!proxy->Bind(0))
	{
		dbg_log("con->Create() failed");
		proxy->Close();
		goto _handle_failed;
	}

	//associated.
	_read_lock(&m_pClient->m_rw_spinlock);
	if (m_pClient->m_flag & FLAG_ASSOCIATED)
	{
		if (m_pClient->m_Iocp->AssociateSock(proxy))
			bAssociated = TRUE;
	}
	_read_unlock(&m_pClient->m_rw_spinlock);
	
	//
	if (!bAssociated)
	{
		goto _handle_failed;
	}

	//
	switch (addrType)
	{
	case 0x1:					// -> IPV4;
		ipv4_addr.sin_family = AF_INET;
		ipv4_addr.sin_port = Port;
		memcpy(&ipv4_addr.sin_addr.S_un.S_addr, addr, 4);
		
		if (!proxy->Connect(
			(SOCKADDR*)&ipv4_addr, 
			sizeof(ipv4_addr),
			NULL,
			NULL))
		{
			proxy->Close();
			goto _handle_failed;
		}
		break;

	case 0x3:
		ipv4_addr.sin_family = AF_INET;
		ipv4_addr.sin_port = Port;

		pHost = gethostbyname((char*)addr);

		if (pHost)
		{
			memcpy(
				&ipv4_addr.sin_addr.S_un.S_addr, 
				pHost->h_addr,
				4);
		}

		if (!proxy->Connect(
			(SOCKADDR*)&ipv4_addr, 
			sizeof(ipv4_addr), 
			NULL, 
			NULL))
		{
			proxy->Close();
			goto _handle_failed;
		}
		break;

	default:
		proxy->Close();
		dbg_log("Client[%d] ConnectHandle invalid addr type: %d", ClientID, addrType);
		goto _handle_failed;
	}

	return;

_handle_failed:

	Error = 1;
	bufs[0].lpData = &ClientID;
	bufs[0].Size = sizeof(ClientID);

	bufs[1].lpData = &Error;
	bufs[1].Size = sizeof(Error);

	Send(SOCK_PROXY_REQUEST_RESULT, bufs, 2);
	Send(SOCK_PROXY_CLOSE, &ClientID, sizeof(ClientID));
}


void CSocksProxy::UDPAssociateHandle(UINT32 ClientID)
{
	CSocksProxyUDP *proxy = new CSocksProxyUDP(this, ClientID);
	BOOL bAssociated = FALSE;
	UINT32 Error;
	vec bufs[2];

	assert(m_Clients[ClientID] == NULL);
	m_Clients[ClientID] = proxy;

	if (!proxy->Create())
	{
		dbg_log("con->Create() failed");
		proxy->Close();
		goto _handle_failed;
	}

	if (!proxy->Bind(0))
	{
		dbg_log("con->Create() failed");
		proxy->Close();
		goto _handle_failed;
	}

	//associated.
	_read_lock(&m_pClient->m_rw_spinlock);
	if (m_pClient->m_flag & FLAG_ASSOCIATED)
	{
		if (m_pClient->m_Iocp->AssociateSock(proxy))
			bAssociated = TRUE;
	}
	_read_unlock(&m_pClient->m_rw_spinlock);

	//
	if (!bAssociated)
	{
		goto _handle_failed;
	}

	proxy->Run();
	return;

_handle_failed:

	Error = 1;
	bufs[0].lpData = &ClientID;
	bufs[0].Size = sizeof(ClientID);

	bufs[1].lpData = &Error;
	bufs[1].Size = sizeof(Error);
	
	Send(SOCK_PROXY_REQUEST_RESULT, bufs, 2);
	Send(SOCK_PROXY_CLOSE, &ClientID, sizeof(ClientID));
}

void CSocksProxy::OnProxyData(BYTE * lpData, UINT Size)
{
	BYTE cmd;
	UINT32 clientID = *(UINT32*)lpData;

	lpData += 4;
	Size -= 4;

	cmd = *lpData;
	++lpData;
	--Size;

	switch (cmd)
	{
	case 0x1:
		((CSocksProxyTcp*)m_Clients[clientID])->OnProxyData(lpData, Size);
		break;
	case 0x3:
		((CSocksProxyUDP*)m_Clients[clientID])->OnProxyData(lpData, Size);
		break;
	default:
		m_Clients[clientID]->Close();
		Send(SOCK_PROXY_CLOSE, &clientID, sizeof(clientID));
		break; 
	}
}

void CSocksProxy::OnProxyRequest(BYTE * lpData, UINT Size)
{
	UINT32 clientID = *(UINT32*)lpData;
	BYTE cmd        = lpData[4];
	BYTE addrType   = lpData[5];
	USHORT port     = *(USHORT*)&lpData[6];
	BYTE * addr     = &lpData[8];

	assert(clientID < MAX_CLIENT_COUNT);
	assert(!m_Clients[clientID]);
	
	HOSTENT*pHost = NULL;

	switch (cmd)
	{
	case 0x01:			//Connect.
		ConnectHandle(clientID, port, addrType, addr);
		break;
	case 0x3:
		UDPAssociateHandle(clientID);
		break;
	default:	
		m_Clients[clientID] = (CIOCPSocket*)-1;
		Send(SOCK_PROXY_CLOSE, NULL, 0);
		dbg_log("");
		break;
	}
}

void CSocksProxy::OnEvent(UINT32 e, BYTE* lpData, UINT Size)
{
	switch (e)
	{
	case SOCK_PROXY_REQUEST:
		OnProxyRequest(lpData, Size);
		break;

	case SOCK_PROXY_DATA:
		OnProxyData(lpData, Size);
		break;

	case SOCK_PROXY_CLOSE:
		OnProxyClose(*(UINT32*)lpData);
		break;
	default:
		break;
	}
}


