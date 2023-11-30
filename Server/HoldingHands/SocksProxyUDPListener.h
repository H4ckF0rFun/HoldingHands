#pragma once
#include "UDPSocket.h"
#include "SocksProxyCommon.h"

class CSocksProxySrv;

class CSocksProxyUDPListener :
	public CUDPSocket
{
private:
	BYTE			 m_buff[UDP_MAX_BUFF];
	BYTE			 m_SendBuf[UDP_MAX_BUFF];

	CSocksProxySrv * m_pHandler;
	UINT32			 m_clientID;

	HANDLE			 m_hEvent;

	BYTE*			 m_clientAddr;
	BYTE			 m_addrLen;

	ULONGLONG		m_traffic[2];	//0: read,1: write;

public:
	BOOL Run();

	//Asnyc operation callback.
	virtual void OnRecvFrom(
		BYTE * lpData,
		UINT32 nTransferredBytes,
		const SOCKADDR *lpFromAddr,
		int iFromLen,
		void * lpParam,
		DWORD Error);

	virtual void OnSendTo(
		BYTE * lpData,
		UINT32 nTransferredBytes,
		const SOCKADDR *lpToAddr,
		int iToLen,
		void * lpParam,
		DWORD Error);

	void OnProxyData(BYTE*lpData, UINT Size);
	
	void GetTraffic(ULONGLONG traffic[2])
	{
		traffic[0] = m_traffic[0];
		traffic[1] = m_traffic[1];
	}

	CSocksProxyUDPListener(CSocksProxySrv*pHandler,UINT32 ClientID);
	virtual ~CSocksProxyUDPListener();
};

