#pragma once

#include "EventHandler.h"
#include "SocksProxyCommon.h"

#define SOCKS_PROXY			(('S') | (('K') << 8) | (('P') << 16) | (('X') << 24))


#define WORK_THREAD_COUNT	8


#define SOCK_PROXY_REQUEST					(0xaa02)


#define SOCK_PROXY_REQUEST_RESULT		(0xaa04)
#define SOCK_PROXY_DATA					(0xaa03)
#define SOCK_PROXY_CLOSE				(0xaa05)


class CIOCPSocket;

class CSocksProxy :
	public CEventHandler
{
private:
	CIOCPSocket *     m_Clients[MAX_CLIENT_COUNT];

public:
	void OnOpen();
	void OnClose();

	void OnProxyRequest(BYTE * lpData,UINT Size);
	void OnProxyData(BYTE * lpData, UINT Size);
	void OnProxyClose(UINT32 ClientID);


	void ConnectHandle(UINT32 ClientID, USHORT  Port, BYTE addrType, BYTE*addr);
	void UDPAssociateHandle(UINT32 ClientID);

	//有数据到达的时候调用这两个函数.
	virtual void OnEvent(UINT32 e, BYTE* lpData, UINT Size);

	static void __stdcall work_thread(CSocksProxy*pThis);
	CSocksProxy(CClient *pClient);
	~CSocksProxy();
};


