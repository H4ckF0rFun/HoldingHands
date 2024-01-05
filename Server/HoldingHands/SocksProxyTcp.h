#pragma once
#include "TCPSocket.h"
#include "socks_proxy_common.h"

/*
	如果 AB 双方都是在析构函数内才put,那么就会造成死锁,无法释放资源
*/

#define WM_SOCKS_PROXY_CONNECTED       (WM_USER + 102)
#define WM_SOCKS_PROXY_CLOSED		   (WM_USER + 103)
#define WM_SOCKS_PROXY_UPDATE		   (WM_USER + 104)

#define SOCKS_PROXY_HANDSHAKE		0
#define SOCKS_PROXY_REQUEST			1
#define SOCKS_PROXY_FORWARD			2


class CSocksProxyUDPListener;
class CSocksProxySrv;
class CSocksProxyTcp :
	public CTCPSocket
{
private:
	int		m_State;
	int     m_Substate;
	
	//handshake
	BYTE	m_Ver;
	BYTE    m_MethodLength;
	BYTE	m_Method[0x100];

	//request.
	BYTE	m_Cmd;
	USHORT	m_ConnectPort;

	BYTE    m_AddrType;
	BYTE    m_AddrLength;
	BYTE	m_ConnectAddr[0x100];

	int		m_ClientID;
	//
	BYTE	m_buff[TCP_MAX_BUFF];
	BYTE*   m_r_ptr;
	BYTE*   m_w_ptr;

	HANDLE	m_hEvent;
	
	//
	BYTE	m_SendBuf[TCP_MAX_BUFF];

	ULONGLONG	m_traffic[2];

	CSocksProxyUDPListener *m_udpListener;
	CSocksProxySrv* m_pHandler;

public:

	BYTE  Cmd() { return m_Cmd; };

	const BYTE* ConnectAddress(){ return m_ConnectAddr; };
	BYTE  ConnectAddrLength(){ return m_AddrLength; };
	BYTE  ConnectAddrType() { return m_AddrType; };
	UINT  ConnectPort(){ return m_ConnectPort; };	
	
	void  GetTraffic(ULONGLONG traffic[2]);
	CSocksProxyUDPListener * GetUdpListener(){ return m_udpListener; };

	void OnProxyResponse(BYTE*lpData,UINT32 Size);
	void OnProxyData(BYTE *lpData,UINT32 Size);

	void SockProxyHandle();
	
	void HandshakeHandle();
	void RequestHandle();
	void TcpForwardHandle();

	void OnOpen();
	void OnClose();

	void OnRecv(BYTE * lpData, UINT32 nTransferredBytes, void * lpParam, DWORD Error);

	CSocksProxyTcp(SOCKET hSocket, CSocksProxySrv *pHandler);
	
	virtual ~CSocksProxyTcp();
};

