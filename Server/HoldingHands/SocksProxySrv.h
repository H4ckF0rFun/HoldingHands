#pragma once
#include "EventHandler.h"
#include "SocksProxyCommon.h"

#define SOCKS_PROXY			(('S') | (('K') << 8) | (('P') << 16) | (('X') << 24))



#define SOCK_PROXY_ADD_CLIENT			(0xaa01)
#define SOCK_PROXY_REQUEST				(0xaa02)
#define SOCK_PROXY_REQUEST_RESULT		(0xaa04)
#define SOCK_PROXY_DATA					(0xaa03)
#define SOCK_PROXY_CLOSE				(0xaa05)



//notify 
#define WM_SOCKS_PROXY_ERROR	(WM_USER + 122)
#define WM_SOCKS_PROXY_LOG		(WM_USER + 123)

class CSocksProxyTcp;
class CSocksProxyListener;
class CIOCP;

class CSocksProxySrv :
	public CEventHandler
{
private:

	//远程未释放的socket 有哪些.
	CSocksProxyTcp* m_clients[MAX_CLIENT_COUNT];
	UINT32			   m_bitmap[MAX_CLIENT_COUNT / 32];
	volatile UINT		 m_free;

	CSocksProxyListener * m_listener;

	UINT32             m_lastID;

	BYTE				m_ver;
	UINT32				m_UDPAssociateAddr;
	void OnProxyResponse(BYTE * lpData, DWORD Size);				//握手阶段
	void OnProxyData(BYTE * lpData, DWORD Size);			//对方发来数据
	void OnProxyClose(DWORD ID);							//对方关闭连接

	spinlock_t m_spinlock;


	volatile ULONGLONG   m_debug_cnt;
public:
	int AllocClientID(CSocksProxyTcp * client);
	void FreeClientID(UINT ClientID);

	DWORD GetConnections();

	BOOL StartProxyServer(UINT Port, const char* IP, const char* UDPAssociateAddr);

	void SetSocksVersion(BYTE Version);
	BYTE GetSocksVersion();

	UINT32 GetUDPAssociateAddr()
	{
		return m_UDPAssociateAddr;
	}

	void debug_inc(){
		InterlockedIncrement(&m_debug_cnt);
	}

	void debug_dec(){
		InterlockedDecrement(&m_debug_cnt);
	}

	void StopProxyServer();

	//
	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen() ;				//当socket连接的时候调用这个函数


	//有数据到达的时候调用这两个函数.
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

	CSocksProxySrv(CClient*pClient);
	~CSocksProxySrv();
};

