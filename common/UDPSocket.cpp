#include "stdafx.h"
#include "UDPSocket.h"
#include "IOCP.h"

CUDPSocket::CUDPSocket()
{

}

CUDPSocket::CUDPSocket(SOCKET hSocket):
	CIOCPSocket(
	hSocket)
{

}

CUDPSocket::~CUDPSocket()
{

}

void CUDPSocket::callback(SOCKET_OVERLAPPED *lpOverlapped, UINT32 nTransferredBytes, DWORD Error)
{
	switch (lpOverlapped->m_IOtype)
	{
	case IO_READ:
		cb_readfrom((UDP_READ_OVERLAPPED*)lpOverlapped, nTransferredBytes, Error);
		break;
	case IO_WRITE:
		cb_sendto((UDP_WRITE_OVERLAPPED*)lpOverlapped, nTransferredBytes, Error);
		break;
	}
}

void CUDPSocket::cb_readfrom(
	UDP_READ_OVERLAPPED *lpOverlapped, 
	UINT32 nTransferredBytes, 
	DWORD Error)
{
	UINT32* lpTranferredBytes = (UINT32*)lpOverlapped->m_lpTransferredByets;
	SOCKADDR *  lpOutFromAddr = lpOverlapped->m_lpOutFromAddr;
	INT*         lpOutFromLen = lpOverlapped->m_lpOutFromLen;

	if (lpTranferredBytes)
	{
		*lpTranferredBytes = nTransferredBytes;
		if (Error)
			*lpTranferredBytes = -Error;
	}

	if (lpOutFromAddr)
	{
		memcpy(lpOutFromAddr, lpOverlapped->m_FromAddr, lpOverlapped->m_iFromLen);
	}

	if (lpOutFromLen)
	{
		*lpOutFromLen = lpOverlapped->m_iFromLen;
	}
	
	//hEvent...
	if (lpOverlapped->m_hEvent)
	{
		SetEvent(lpOverlapped->m_hEvent);
		goto __release__;
	}

	OnRecvFrom(
		lpOverlapped->m_lpData, 
		nTransferredBytes, 
		(SOCKADDR*)lpOverlapped->m_FromAddr, 
		lpOverlapped->m_iFromLen, 
		lpOverlapped->m_lpParam, 
		Error);

__release__:
	delete lpOverlapped;
}

void CUDPSocket::cb_sendto(
	UDP_WRITE_OVERLAPPED *lpOverlapped,
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

	OnSendTo(
		lpOverlapped->m_lpData,
		nTransferredBytes,
		(SOCKADDR*)lpOverlapped->m_ToAddr,
		lpOverlapped->m_iToLen,
		lpOverlapped->m_lpParam,
		Error);

__release__:
	delete lpOverlapped;
}


BOOL CUDPSocket::RecvFrom(
	BYTE * lpData,
	UINT32 Size,
	UINT32 *lpTransferredBytes,
	SOCKADDR * lpFrom,
	PINT32 lpFromLen,
	void * lpParam,
	HANDLE hEvent)
{
	BOOL bRet = FALSE;
	UDP_READ_OVERLAPPED * lpOverlapped;  

	_read_lock(&m_rw_spinlock);

	if (m_flag & (FLAG_CLOSING | FLAG_CLOSED))
		goto __leave__;

	if (!(m_flag & FLAG_ASSOCIATED))
		goto __leave__;
	
	lpOverlapped = new UDP_READ_OVERLAPPED(
		lpData,
		Size, 
		lpTransferredBytes, 
		(SOCKADDR*)lpFrom,
		lpFromLen,
		lpParam,
		hEvent);
	
	bRet = m_Iocp->PostRecvFrom(
		this,
		lpData,
		Size,
		(SOCKADDR*)&lpOverlapped->m_FromAddr,
		&lpOverlapped->m_iFromLen ,
		lpOverlapped);

	if (!bRet)
		delete lpOverlapped;

__leave__:
	_read_unlock(&m_rw_spinlock);

	if (!bRet && hEvent)
		SetEvent(hEvent);

	return bRet;
}

BOOL CUDPSocket::SendTo(
	BYTE * lpData,
	UINT32 Size,
	UINT32 *lpTransferredBytes,
	const SOCKADDR * lpTo,
	INT32  iToLen,
	void * lpParam,
	HANDLE hEvent)
{
	BOOL bRet = FALSE;
	UDP_WRITE_OVERLAPPED * lpOverlapped;

	_read_lock(&m_rw_spinlock);

	if (m_flag & (FLAG_CLOSING | FLAG_CLOSED))
		goto __leave__;

	if (!(m_flag & FLAG_ASSOCIATED))
		goto __leave__;

	lpOverlapped = new UDP_WRITE_OVERLAPPED(
		lpData,
		Size,
		lpTransferredBytes,
		(const SOCKADDR*)lpTo,
		iToLen,
		lpParam,
		hEvent);

	bRet = m_Iocp->PostSendTo(
		this,
		lpData,
		Size,
		(const SOCKADDR*)&lpOverlapped->m_ToAddr,
		lpOverlapped->m_iToLen,
		lpOverlapped);

	if (!bRet)
		delete lpOverlapped; 

__leave__:
	_read_unlock(&m_rw_spinlock);

	if (!bRet && hEvent)
		SetEvent(hEvent);

	return bRet; 
}