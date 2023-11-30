#include "SocksProxyTcp.h"
#include "SocksProxy.h"

CSocksProxyTcp::CSocksProxyTcp(CSocksProxy*pHandler, UINT32 ClientID)
{
	m_pHandler = pHandler;
	m_pHandler->Get();
	m_ClientID = ClientID;

	m_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
}


CSocksProxyTcp::~CSocksProxyTcp()
{
	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	m_pHandler->Put();
}

void CSocksProxyTcp::OnRecv(BYTE * lpData, UINT32 nTransferredBytes, void * lpParam, DWORD Error)
{
	vec bufs[2];
	if (Error || !nTransferredBytes)
	{
		goto __closed__;
	}

	//
	bufs[0].lpData = &m_ClientID;
	bufs[0].Size = sizeof(m_ClientID);
	bufs[1].lpData = lpData;
	bufs[1].Size = nTransferredBytes;

	m_pHandler->Send(SOCK_PROXY_DATA, bufs, 2);

	//next forward .
	if (!Recv(m_buff, sizeof(m_buff), NULL))
	{
		goto __closed__;
	}
	return;

__closed__:
	m_pHandler->Send(SOCK_PROXY_CLOSE, &m_ClientID, sizeof(m_ClientID));
	Close();
	return;
}

void CSocksProxyTcp::OnProxyData(BYTE *lpData, UINT Size)
{
	WaitForSingleObject(m_hEvent, INFINITE);
	memcpy(m_SendBuff, lpData, Size);
	Send(m_SendBuff, Size, NULL, NULL, m_hEvent);
}

void CSocksProxyTcp::OnConnect(const SOCKADDR *Addr, int iAddrLen, void * lpParam, DWORD Error)
{
	vec bufs[2];
	bufs[0].lpData = &m_ClientID;
	bufs[0].Size = sizeof(m_ClientID);

	bufs[1].lpData = &Error;
	bufs[1].Size = sizeof(Error);

	m_pHandler->Send(SOCK_PROXY_REQUEST_RESULT,bufs,2);

	dbg_log("client[%d] OnConnect ,error L: %d",m_ClientID ,Error);

	if (Error)
	{	
		m_pHandler->Send(SOCK_PROXY_CLOSE,&m_ClientID,sizeof(m_ClientID));
		Close();
		return;
	}

	//begin forward .
	if (!Recv(m_buff, sizeof(m_buff), NULL))
	{
		m_pHandler->Send(SOCK_PROXY_CLOSE, &m_ClientID, sizeof(m_ClientID));
		Close();
		return;
	}
}