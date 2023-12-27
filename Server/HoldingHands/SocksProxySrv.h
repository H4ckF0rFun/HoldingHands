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

	//Զ��δ�ͷŵ�socket ����Щ.
	CSocksProxyTcp* m_clients[MAX_CLIENT_COUNT];
	UINT32			   m_bitmap[MAX_CLIENT_COUNT / 32];
	volatile UINT		 m_free;

	CSocksProxyListener * m_listener;

	UINT32             m_lastID;

	BYTE				m_ver;
	UINT32				m_UDPAssociateAddr;
	void OnProxyResponse(BYTE * lpData, DWORD Size);				//���ֽ׶�
	void OnProxyData(BYTE * lpData, DWORD Size);			//�Է���������
	void OnProxyClose(DWORD ID);							//�Է��ر�����

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
	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnOpen() ;				//��socket���ӵ�ʱ������������


	//�����ݵ����ʱ���������������.
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

	CSocksProxySrv(CClient*pClient);
	~CSocksProxySrv();
};

