#pragma once
#include "UDPSocket.h"
#include "SocksProxyCommon.h"

class CSocksProxy;
class CSocksProxyUDP :
	public CUDPSocket
{
private:
	CSocksProxy * m_pHandler;
	UINT32        m_ClientID;
	BYTE          m_buff[UDP_MAX_BUFF];
	BYTE          m_SendBuff[UDP_MAX_BUFF];
	HANDLE        m_hEvent;

public:
	void OnRecvFrom(
		BYTE * lpData,
		UINT32 nTransferredBytes,
		const SOCKADDR *lpFromAddr,
		int iFromLen,
		void * lpParam,
		DWORD Error);

	void OnProxyData(BYTE *lpData, UINT Size);
	void Run();
	CSocksProxyUDP(CSocksProxy*pHandler, UINT32 ClientID);
	virtual ~CSocksProxyUDP();
};

