#include "SocksProxyUDP.h"
#include "SocksProxy.h"

CSocksProxyUDP::CSocksProxyUDP(CSocksProxy*pHandler, UINT32 ClientID)
{
	m_ClientID = ClientID;
	m_pHandler = pHandler;
	m_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	m_pHandler->Get();
}


CSocksProxyUDP::~CSocksProxyUDP()
{
	m_pHandler->Put();
	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
}


void CSocksProxyUDP::OnRecvFrom(
	BYTE * lpData,
	UINT32 nTransferredBytes,
	const SOCKADDR *lpFromAddr,
	int iFromLen,
	void * lpParam,
	DWORD Error)
{
	vec bufs[2] = { 0 };
	BYTE atyp;

	if (Error || !nTransferredBytes)
	{
		m_pHandler->Send(SOCK_PROXY_CLOSE, &m_ClientID, sizeof(m_ClientID));
		Close();
		return;
	}

	bufs[0].lpData = &m_ClientID;
	bufs[0].Size = sizeof(m_ClientID);

	bufs[1].lpData = lpData;
	bufs[1].Size = nTransferredBytes;

	m_pHandler->Send(SOCK_PROXY_DATA, bufs, 2);

	if (!RecvFrom(m_buff, sizeof(m_buff), NULL, NULL, NULL))
	{
		m_pHandler->Send(SOCK_PROXY_CLOSE, &m_ClientID, sizeof(m_ClientID));
		Close();
	}
}
/*
	ATYP		1				01: IPV4
								03: domain
								04: IPV6
	DST.PORT	2				dest.port

	DATA		xxx
*/

void CSocksProxyUDP::OnProxyData(BYTE *lpData, UINT Size)
{
	BYTE addrtype = *lpData;
	SOCKADDR_IN ipv4addr = { 0 };
	BYTE        addrlen;
	HOSTENT *   host;
	char domain[257] = { 0 };

	++lpData;
	--Size;

	switch (addrtype)
	{
	case 0x1:
		ipv4addr.sin_family = AF_INET;
		memcpy(&ipv4addr.sin_addr, lpData, 4);
		lpData += 4;
		Size -= 4;

		ipv4addr.sin_port = *(UINT16*)lpData;
		lpData += 2;
		Size -= 2;

		WaitForSingleObject(m_hEvent, INFINITE);
		memcpy(m_SendBuff, lpData, Size);
		SendTo(m_SendBuff, Size, NULL, (SOCKADDR*)&ipv4addr, sizeof(ipv4addr), NULL, m_hEvent);

		break;
	case 0x3:		//domain
		addrlen = *lpData;
		++lpData;
		--Size;

		memcpy(domain, lpData, addrlen);
		domain[addrlen] = 0;

		lpData += addrlen;
		Size -= addrlen;

		host = gethostbyname(domain);

		if (host)
		{
			ipv4addr.sin_family = AF_INET;
			memcpy(&ipv4addr.sin_addr, host->h_addr, 4);
			ipv4addr.sin_port = *(UINT16*)lpData;
			lpData += 2;
			Size -= 2;

			WaitForSingleObject(m_hEvent, INFINITE);
			memcpy(m_SendBuff, lpData, Size);
			SendTo(m_SendBuff, Size, NULL, (SOCKADDR*)&ipv4addr, sizeof(ipv4addr), NULL, m_hEvent);
		}
		break;

	default:
		dbg_log("UDP Associate : unsupported addr type : %d", addrtype);
		Close();
		break;
	}
}

void CSocksProxyUDP::Run()
{
	vec bufs[2];
	DWORD Error = 0;

	bufs[0].lpData = &m_ClientID;
	bufs[0].Size = sizeof(m_ClientID);

	bufs[1].lpData = &Error;
	bufs[1].Size = sizeof(Error);

	m_pHandler->Send(SOCK_PROXY_REQUEST_RESULT, bufs, 2);

	if (!RecvFrom(m_buff, sizeof(m_buff), NULL,NULL,0))
	{
		m_pHandler->Send(SOCK_PROXY_CLOSE, &m_ClientID, sizeof(m_ClientID));
		Close();
	}
}