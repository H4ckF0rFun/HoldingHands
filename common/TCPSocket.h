#pragma once
#include "IOCPSocket.h"
#include "spinlock.h"



#define IO_READ    0
#define IO_WRITE   1
#define IO_CONNECT 2
#define IO_ACCEPT  3

struct TCP_READ_OVERLAPPED : SOCKET_OVERLAPPED
{
	HANDLE		m_hEvent;
	UINT8 *	    m_lpData;
	UINT32      m_ReqSize;
	UINT32 *	m_lpTransferredByets;
	LPVOID      m_lpParam;

	TCP_READ_OVERLAPPED(
		UINT8 *lpData, 
		UINT32 ReqSize, 
		UINT32 *lpTransferredByets,
		LPVOID  lpParam,
		HANDLE hEvent) :
		SOCKET_OVERLAPPED(IO_READ)
	{
		m_hEvent = hEvent;
		m_lpData = lpData;
		m_ReqSize = ReqSize; 
		m_lpTransferredByets = lpTransferredByets;
		m_lpParam = lpParam;
	}
};



struct TCP_WRITE_OVERLAPPED : SOCKET_OVERLAPPED
{
	HANDLE		m_hEvent;
	UINT8 *	    m_lpData;
	UINT32      m_ReqSize;
	UINT32 *	m_lpTransferredByets;
	LPVOID      m_lpParam;

	TCP_WRITE_OVERLAPPED(
		UINT8 *lpData,
		UINT32 ReqSize,
		UINT32 *lpTransferredByets,
		LPVOID  lpParam,
		HANDLE hEvent) :
		SOCKET_OVERLAPPED(IO_WRITE)
	{
		m_hEvent = hEvent;
		m_lpData = lpData;
		m_ReqSize = ReqSize;
		m_lpTransferredByets = lpTransferredByets;
		m_lpParam = lpParam;
	}
};


struct TCP_ACCEPT_OVERLAPPED : SOCKET_OVERLAPPED
{
	HANDLE		m_hEvent;
	UINT8       m_AcceptAddrBuf[256];			
	/*
		uint8 local_addr;
		uint8 remote_addr;
	*/
	SOCKET      m_hAcceptSocket;
	SOCKET*     m_lpAcceptedSocketOut;
	LPVOID      m_lpParam;

	TCP_ACCEPT_OVERLAPPED(
		SOCKET hAcceptSocket,
		SOCKET* lpAcceptedSocketOut,
		LPVOID  lpParam,
		HANDLE hEvent) :
		SOCKET_OVERLAPPED(IO_ACCEPT)
	{
		m_hEvent = hEvent;
		m_hAcceptSocket = hAcceptSocket;
		m_lpAcceptedSocketOut = lpAcceptedSocketOut;
		m_lpParam = lpParam;
		ZeroMemory(m_AcceptAddrBuf,sizeof(m_AcceptAddrBuf));
	}
};

struct TCP_CONNECT_OVERLAPPED : SOCKET_OVERLAPPED
{
	HANDLE		m_hEvent;
	char     	m_Addr[MAX_ADDR_LEN];
	int         m_iAddrLen;
	LPVOID      m_lpParam;

	TCP_CONNECT_OVERLAPPED(
		const SOCKADDR * Addr,
		int iAddrLen,
		LPVOID  lpParam,
		HANDLE hEvent) :
		SOCKET_OVERLAPPED(IO_CONNECT)
	{
		m_hEvent = hEvent;
		m_iAddrLen = iAddrLen;
		memcpy(&m_Addr, Addr, iAddrLen);
		m_lpParam = lpParam;
	}
};


class CTCPSocket :
	public CIOCPSocket
{
public:
	CTCPSocket();
	CTCPSocket(SOCKET hSocket,DWORD flag = 0);

	virtual ~CTCPSocket();

	//Asnyc operation callback.
	virtual void OnRecv(BYTE * lpData, UINT32 nTransferredBytes, void * lpParam, DWORD Error){};
	virtual void OnSend(BYTE * lpData, UINT32 nTransferredBytes, void * lpParam, DWORD Error){};
	virtual void OnAccept(SOCKET hClientSocket,const SOCKADDR* RemoteAddr,int AddrLen ,void * lpParam, DWORD Error){};
	virtual void OnConnect(const SOCKADDR *Addr, int iAddrLen, void * lpParam, DWORD Error){};

private:
	void callback(SOCKET_OVERLAPPED*lpOverlapped, UINT32 nTransferredBytes, DWORD Error);
	void cb_read   (TCP_READ_OVERLAPPED *lpOverlapped, UINT32 nTransferredBytes, DWORD Error);
	void cb_write  (TCP_WRITE_OVERLAPPED *lpOverlapped, UINT32 nTransferredBytes, DWORD Error);
	void cb_accept (TCP_ACCEPT_OVERLAPPED * lpOverlapped ,DWORD Error);
	void cb_connect(TCP_CONNECT_OVERLAPPED *lpOverlapped,DWORD Error);

public:

	BOOL Connect(const SOCKADDR * Addr, int AddrLen, void *lpParam, HANDLE hEvent = NULL);
	BOOL Connect(const char * Addr, USHORT Port, void *lpParam, HANDLE hEvent = NULL);
	
	BOOL IsConnected()
	{
		BOOL bConnected = FALSE;
		_read_lock(&m_rw_spinlock);
		bConnected = (m_flag & FLAG_CONNECTED);
		_read_unlock(&m_rw_spinlock);

		return bConnected;
	}

	BOOL Accept(SOCKET* hClientSocket, void * lpParam = NULL, HANDLE hEvent = NULL);
	BOOL Listen(int backlog);

	//Async Recv and Send.
	BOOL Recv(BYTE * lpData, UINT32 Size, UINT *lpRecvBytes, void * lpParam = NULL, HANDLE hEvent = NULL);
	BOOL Send(BYTE * lpData, UINT32 Size, UINT *lpSendBytes, void * lpParam = NULL, HANDLE hEvent = NULL);
	//
	BOOL Create()
	{
		BOOL bRet = FALSE;

		_write_lock(&m_rw_spinlock);

		if (m_flag & FLAG_CLOSED)
		{
			m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
			bRet = (m_hSocket != INVALID_SOCKET);
			
			if (bRet)
			{
				m_flag &= (~FLAG_CLOSED);
				m_flag |= FLAG_CREATED;
			}
		}
		_write_unlock(&m_rw_spinlock);
		return bRet;
	}

	BOOL GetPeerName(SOCKADDR * addr, int * addrlen)
	{
		BOOL bRet = FALSE;
		_read_lock(&m_rw_spinlock);

		//getpeername only this socket is connected or accepted socket..
		if (m_flag & (FLAG_CONNECTED | FLAG_ACCEPTED))
		{
			bRet = (getpeername(m_hSocket, addr, addrlen) == 0);
		}
		_read_unlock(&m_rw_spinlock);
		return bRet;
	}

	static LPFN_GETACCEPTEXSOCKADDRS lpGetAcceptExSockaddrs;
};

