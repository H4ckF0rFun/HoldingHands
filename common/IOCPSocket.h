#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include "IOCP.h"
#include "list.h"
#include "spinlock.h"
#include "dbg.h"

class CIOCP;

#define IO_READ    0

#define IO_WRITE   1
#define IO_ACCEPT  2

#define IO_CONNECT 3


#define MAX_ADDR_LEN  128

struct SOCKET_OVERLAPPED : OVERLAPPED
{
	DWORD		m_IOtype;
	SOCKET_OVERLAPPED(DWORD IOType)
	{
		ZeroMemory(this, sizeof(OVERLAPPED));
		m_IOtype = IOType;
	}
};


//can listen,bind.associate
#define FLAG_CREATED     0x1

#define FLAG_CLOSING     0x2
#define FLAG_CLOSED      0x4

//can accept .associate
#define FLAG_LISTENING   0x8

//can .associate
#define FLAG_CONNECTING  0x10

//can associate,send ,recv
#define FLAG_CONNECTED	 0x20
#define FLAG_ACCEPTED	 0x40

//associated.
#define FLAG_ASSOCIATED  0x80

class CIOCPSocket
{

public:
	volatile  LONG            m_flag;
	CIOCP *                   m_Iocp;
	SOCKET					  m_hSocket;

	volatile unsigned long    m_RefCount;			//引用计数.
	rw_spinlock_t             m_rw_spinlock ;
	
	//volatile unsigned long    m_lock;

	virtual ~CIOCPSocket()
	{
		Close();
		dbg_log("~CIOCPSocket()");
	}

public:
	list_node                 m_list;

	CIOCPSocket* Get(){				//thread-safe.
		InterlockedIncrement(&m_RefCount);
		return this;
	}

	void		 Put(){				//thread-safe.
		if (!InterlockedDecrement(&m_RefCount)){
			delete this;
		}
	}

	//
	CIOCPSocket()
	{
		m_hSocket = INVALID_SOCKET;
		m_Iocp = NULL;
		m_flag = FLAG_CLOSED;

		//initialize rw spinlock.
		_rw_spinlock_init(&m_rw_spinlock);

		//initialize ref count.
		InterlockedExchange(&m_RefCount, 1);
	}

	CIOCPSocket(SOCKET hSock,DWORD initialFlag=0)
	{
		m_hSocket  = hSock;
		m_Iocp     = NULL;
		m_flag     = FLAG_CREATED;
		
		if (initialFlag)
			m_flag |= initialFlag;
		
		//initialize rw spinlock.
		_rw_spinlock_init(&m_rw_spinlock);

		//initialize ref count.
		InterlockedExchange(&m_RefCount, 1);
	}

	SOCKET GetSocket() const { return m_hSocket; };		//this socket maybe close when you get the return value.
	

	void Close()						//thread-safe.
	{
		BOOL bAssociated = FALSE;
		_write_lock(&m_rw_spinlock);

		if (m_flag & (FLAG_CLOSING | FLAG_CLOSED))
		{
			//sock has been closed.
			_write_unlock(&m_rw_spinlock);
			return;
		}

		m_flag |= FLAG_CLOSING;

		bAssociated = m_flag & FLAG_ASSOCIATED;

		m_flag &= (~(FLAG_CREATED | FLAG_ASSOCIATED));
		_write_unlock(&m_rw_spinlock);
		
		//we should wait all one to release his safe socket,then close socket.
		//now,there will no io request, 
		//and no one will use our socket.
		
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;

		//now there will no 
		if (bAssociated)
		{
			m_Iocp->RemoveSock(this);
			m_Iocp = NULL;
		}

		_write_lock(&m_rw_spinlock);
		m_flag = FLAG_CLOSED;
		_write_unlock(&m_rw_spinlock);
	}

	BOOL Bind(const SOCKADDR * addr, int addrLen)
	{
		BOOL bRet = FALSE;
		
		_read_lock(&m_rw_spinlock);
		//只有一个刚创建的socket才能bind address.
		if (!(m_flag & (~(FLAG_CREATED | FLAG_ASSOCIATED))))
		{
			bRet = (bind(m_hSocket, addr, addrLen) == 0);
		}
		_read_unlock(&m_rw_spinlock);
		
		return bRet;
	}

	BOOL Bind(UINT32 Port,const char * Address = NULL)
	{
		SOCKADDR_IN addr = { 0 };
		BOOL bRet = FALSE;
		addr.sin_port = htons(Port);
		addr.sin_family = AF_INET;
		
		if (Address)
		{
			addr.sin_addr.S_un.S_addr = inet_addr(Address);
		}

		bRet = Bind((SOCKADDR*)&addr, sizeof(addr));

		return bRet;
	}

	//tcp and udp both can call get sock name,but only tcp can call get peer name,
	BOOL GetSockName(SOCKADDR * sockaddr, int * addrlen)
	{
		BOOL bRet;

		_read_lock(&m_rw_spinlock);

		//getsockname only this socket is not closed.
		if (m_flag & (FLAG_CLOSING | FLAG_CLOSED))
		{
			_read_unlock(&m_rw_spinlock);
			return FALSE;
		}

		bRet = (getsockname(m_hSocket, sockaddr, addrlen) == 0);
		_read_unlock(&m_rw_spinlock);

		return bRet;
	}
	//Asnyc operation callback.
	virtual void callback(SOCKET_OVERLAPPED *lpOverlapped, UINT32 nTransferredBytes, DWORD Error) = 0;
};

