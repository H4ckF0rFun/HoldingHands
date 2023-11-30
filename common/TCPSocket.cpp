#include "stdafx.h"
#include "TCPSocket.h"
#include "IOCP.h"

#pragma comment(lib,"Mswsock.lib")

LPFN_GETACCEPTEXSOCKADDRS CTCPSocket::lpGetAcceptExSockaddrs = NULL;

CTCPSocket::CTCPSocket()
{
}

CTCPSocket::CTCPSocket(SOCKET hSocket,DWORD flag):
	CIOCPSocket(
	hSocket, flag)
{
}

CTCPSocket::~CTCPSocket()
{	

}

void CTCPSocket::callback(SOCKET_OVERLAPPED*lpOverlapped, UINT32 nTransferredBytes, DWORD Error)
{
	switch (lpOverlapped->m_IOtype)
	{
	case IO_READ:
		cb_read((TCP_READ_OVERLAPPED*)lpOverlapped, nTransferredBytes, Error);
		break;
	case IO_WRITE:
		cb_write((TCP_WRITE_OVERLAPPED*)lpOverlapped, nTransferredBytes, Error);
		break;
	case IO_ACCEPT:
		cb_accept((TCP_ACCEPT_OVERLAPPED*)lpOverlapped, Error);
		break;
	case IO_CONNECT:
		cb_connect((TCP_CONNECT_OVERLAPPED*)lpOverlapped, Error);
		break;
	}
}


void CTCPSocket::cb_read(
	TCP_READ_OVERLAPPED *lpOverlapped, 
	UINT32 nTransferredBytes, 
	DWORD Error)
{
	UINT32* lpTranferredBytes = (UINT32*)lpOverlapped->m_lpTransferredByets;

	if (lpTranferredBytes)
	{
		*lpTranferredBytes = nTransferredBytes;
		if (Error)
			*lpTranferredBytes = -Error;
	}

	//hEvent...
	if (lpOverlapped->m_hEvent)
	{
		SetEvent(lpOverlapped->m_hEvent);
		goto __release__;
	}


	OnRecv(lpOverlapped->m_lpData, nTransferredBytes, lpOverlapped->m_lpParam, Error);

__release__:
	delete lpOverlapped;
}

void CTCPSocket::cb_write(TCP_WRITE_OVERLAPPED *lpOverlapped, UINT32 nTransferredBytes, DWORD Error)
{
	UINT32* lpTranferredBytes = (UINT32*)lpOverlapped->m_lpTransferredByets;

	if (lpTranferredBytes)
	{
		*lpTranferredBytes = nTransferredBytes;
		if (Error)
			*lpTranferredBytes = -Error;
	}

	//hEvent...
	if (lpOverlapped->m_hEvent)
	{
		SetEvent(lpOverlapped->m_hEvent);
		goto __release__;
	}

	OnSend(lpOverlapped->m_lpData, nTransferredBytes, lpOverlapped->m_lpParam, Error);

__release__:
	delete lpOverlapped;
}

void CTCPSocket::cb_accept(TCP_ACCEPT_OVERLAPPED *lpOverlapped, DWORD Error)
{
	SOCKET  hSock = lpOverlapped->m_hAcceptSocket;
	SOCKET* lpOutSock = lpOverlapped->m_lpAcceptedSocketOut;
	
	SOCKADDR *lpRemoteAddr = 0;
	SOCKADDR *lpLocalAddr = 0;

	int iLocalAddrLength = 0;
	int iRemoteAddrLength = 0;

	if (Error)
	{
		closesocket(hSock);
		hSock = INVALID_SOCKET;
	}

	if (hSock != INVALID_SOCKET)
	{
		setsockopt(
			hSock,
			SOL_SOCKET,
			SO_UPDATE_ACCEPT_CONTEXT,
			(char *)&m_hSocket,
			sizeof(m_hSocket));
	}

	if (lpOutSock)
		*lpOutSock = hSock;

	if (lpOverlapped->m_hEvent)
	{
		SetEvent(lpOverlapped->m_hEvent);
		goto __release__;
	}

	if (!lpGetAcceptExSockaddrs)
	{
		INT iResult = INVALID_SOCKET;
		SOCKET temp = socket(AF_INET, SOCK_STREAM, NULL);
		DWORD dwBytes = 0;
		if (temp != INVALID_SOCKET)
		{
			GUID tmpGuid  = WSAID_GETACCEPTEXSOCKADDRS;
			iResult = WSAIoctl(
				temp,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&tmpGuid, sizeof(tmpGuid),
				&lpGetAcceptExSockaddrs, sizeof(lpGetAcceptExSockaddrs),
				&dwBytes,
				NULL,
				NULL);

			closesocket(temp);
		}
	}

	if (lpGetAcceptExSockaddrs)
	{
		lpGetAcceptExSockaddrs(
			lpOverlapped->m_AcceptAddrBuf,
			0,
			MAX_ADDR_LEN,
			MAX_ADDR_LEN,
			&lpLocalAddr,
			&iLocalAddrLength,
			&lpRemoteAddr,
			&iRemoteAddrLength);
	}
	

	OnAccept(
		hSock, 
		lpRemoteAddr,
		iRemoteAddrLength, 
		lpOverlapped->m_lpParam, 
		Error);

__release__:
	delete lpOverlapped;
}

void CTCPSocket::cb_connect(TCP_CONNECT_OVERLAPPED *lpOverlapped,DWORD Error)
{
	if (!Error)
	{
		_write_lock(&m_rw_spinlock);
		m_flag |= FLAG_CONNECTED;
		_write_unlock(&m_rw_spinlock);

		setsockopt(m_hSocket,
			SOL_SOCKET,
			SO_UPDATE_CONNECT_CONTEXT,
			NULL,
			0);
	}
	else
	{
		_write_lock(&m_rw_spinlock);
		m_flag &= (~FLAG_CONNECTING);
		_write_unlock(&m_rw_spinlock);
	}


	if (lpOverlapped->m_hEvent)
	{
		SetEvent(lpOverlapped->m_hEvent);
		goto __release__;
	}

	OnConnect(
		(SOCKADDR*)&lpOverlapped->m_Addr, 
		lpOverlapped->m_iAddrLen, 
		lpOverlapped->m_lpParam, 
		Error);

__release__:
	delete lpOverlapped;
}

//Async Recv.
BOOL CTCPSocket::Recv(
	UINT8* lpData, 
	UINT32 Size, 
	UINT * lpRecvBytes, 
	void * lpParam ,
	HANDLE hEvent)
{
	BOOL bRet = FALSE;
	TCP_READ_OVERLAPPED * pOverlapped = NULL;

	_read_lock(&m_rw_spinlock);

	if (!(m_flag & (FLAG_CONNECTED | FLAG_ACCEPTED)))
		goto __leave__;

	if (!(m_flag & FLAG_ASSOCIATED))
		goto __leave__;


	pOverlapped = new TCP_READ_OVERLAPPED(
		lpData,
		Size,
		lpRecvBytes,
		lpParam,
		hEvent);

	bRet = m_Iocp->PostRecv(this, lpData, Size, pOverlapped);
	
	if (!bRet)
		delete pOverlapped;

__leave__:
	_read_unlock(&m_rw_spinlock);
	
	if (!bRet && hEvent)
		SetEvent(hEvent);

	return bRet;
}


BOOL CTCPSocket::Send(
	BYTE * lpData, 
	UINT32 Size,
	UINT *lpSendBytes,
	void * lpParam,
	HANDLE hEvent)
{
	BOOL bRet = FALSE;
	TCP_WRITE_OVERLAPPED * lpOverlapped;


	_read_lock(&m_rw_spinlock);
	//only these flags is set,we can send data.
	if (!(m_flag & (FLAG_CONNECTED |FLAG_ACCEPTED)))
		goto __leave__;

	if (!(m_flag & FLAG_ASSOCIATED))
		goto __leave__;

	lpOverlapped = new TCP_WRITE_OVERLAPPED(
		lpData,
		Size,
		lpSendBytes,
		lpParam,
		hEvent);

	bRet = m_Iocp->PostSend(this, lpData, Size, lpOverlapped);
	if (!bRet)
	{
		delete lpOverlapped;
	}

__leave__:
	_read_unlock(&m_rw_spinlock);

	if (!bRet && hEvent)
		SetEvent(hEvent);

	return bRet;
}

BOOL CTCPSocket::Connect(const SOCKADDR * Addr, int AddrLen, void *lpParam, HANDLE hEvent)
{
	BOOL bRet = FALSE;
	TCP_CONNECT_OVERLAPPED *lpOverlapped;

	_write_lock(&m_rw_spinlock);

	if (m_flag & (~(FLAG_CREATED | FLAG_ASSOCIATED)))
		goto __leave__;

	lpOverlapped = new TCP_CONNECT_OVERLAPPED(
		Addr,
		AddrLen,
		lpParam,
		hEvent);

	m_flag |= FLAG_CONNECTING;

	bRet = m_Iocp->PostConnect(
		this, 
		(SOCKADDR*)lpOverlapped->m_Addr, 
		lpOverlapped->m_iAddrLen, 
		lpOverlapped);

	if (!bRet)
	{
		delete lpOverlapped;
		m_flag &= (~FLAG_CONNECTING);
	}

__leave__:
	_write_unlock(&m_rw_spinlock);

	if (!bRet && hEvent)
		SetEvent(hEvent);

	return bRet;
}


BOOL CTCPSocket::Accept(SOCKET * hClientSocket, void * lpParam, HANDLE hEvent)
{
	UINT   nRecvBytes = 0;
	TCP_ACCEPT_OVERLAPPED *lpOverlapped;
	SOCKET hTempSocket;
	BOOL   bRet = FALSE;

	_read_lock(&m_rw_spinlock);

	//accept only these flags is set.
	if (m_flag & (~(FLAG_CREATED | FLAG_LISTENING |FLAG_ASSOCIATED)))
		goto __leave__;

	hTempSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (hTempSocket == INVALID_SOCKET)
		goto __leave__;

	lpOverlapped = new TCP_ACCEPT_OVERLAPPED(
		hTempSocket,
		hClientSocket,
		lpParam,
		hEvent);

	bRet = m_Iocp->PostAccept(
		this,
		hTempSocket,
		lpOverlapped->m_AcceptAddrBuf,
		&nRecvBytes,
		MAX_ADDR_LEN,
		MAX_ADDR_LEN,
		lpOverlapped);
	
	if (!bRet)
	{
		closesocket(hTempSocket);
		delete lpOverlapped;
	}

__leave__:
	_read_unlock(&m_rw_spinlock);

	if (!bRet && hEvent)
		SetEvent(hEvent);

	return bRet;
}


BOOL CTCPSocket::Connect(const char * Addr, USHORT Port, void *lpParam, HANDLE hEvent)
{
	SOCKADDR_IN addr = { 0 };
	HOSTENT * lpHost = gethostbyname(Addr);

	if (lpHost == NULL)
		return FALSE;

	memcpy(&addr.sin_addr.S_un.S_addr, lpHost->h_addr, 4);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Port);

	return Connect((SOCKADDR*)&addr, sizeof(addr), lpParam, hEvent);
}


BOOL CTCPSocket::Listen(int backlog)
{
	BOOL bRet = FALSE;
	_write_lock(&m_rw_spinlock);

	//只有一个刚创建的socket 才能 listen
	if (m_flag & (~(FLAG_CREATED | FLAG_ASSOCIATED)))
	{
		_write_unlock(&m_rw_spinlock);
		return FALSE; 
	}

	bRet = listen(m_hSocket, backlog) != -1;

	if (bRet)
		m_flag |= FLAG_LISTENING;

	_write_unlock(&m_rw_spinlock);
	return bRet;
}
