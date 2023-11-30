#pragma once
#include "TCPSocket.h"
#include "SocksProxyCommon.h"

class CSocksProxy;

class CSocksProxyTcp :
	public CTCPSocket
{
private:
	CSocksProxy * m_pHandler;
	UINT32 m_ClientID;
	HANDLE  m_hEvent;
	BYTE	m_buff[TCP_MAX_BUFF];

	BYTE	m_SendBuff[TCP_MAX_BUFF];
public:

	void OnRecv(BYTE * lpData, UINT32 nTransferredBytes, void * lpParam, DWORD Error);
	void OnProxyData(BYTE *lpData, UINT Size);

	void OnConnect(const SOCKADDR *Addr, int iAddrLen, void * lpParam, DWORD Error);


	CSocksProxyTcp(CSocksProxy*pHandler,UINT32 ClientID);
	virtual ~CSocksProxyTcp();
};

