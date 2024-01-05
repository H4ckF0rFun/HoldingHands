#pragma once

#include "EventHandler.h"
#include "socks_proxy_common.h"
#include "module.h"

#define WORK_THREAD_COUNT	8


class CIOCPSocket;

class CSocksProxy :
	public CEventHandler
{
private:
	CIOCPSocket *     m_Clients[MAX_CLIENT_COUNT];
	Module      * m_owner;
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
	CSocksProxy(CClient *pClient,Module * owner);
	~CSocksProxy();
};


