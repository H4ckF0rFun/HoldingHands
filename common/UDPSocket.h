#pragma once
#include "IOCPSocket.h"


struct UDP_READ_OVERLAPPED : SOCKET_OVERLAPPED
{
	UINT8 *	    m_lpData;
	UINT32      m_ReqSize;
	UINT32 *	m_lpTransferredByets;
	SOCKADDR	*m_lpOutFromAddr;
	int*        m_lpOutFromLen;
	LPVOID      m_lpParam;
	HANDLE      m_hEvent;

	char		m_FromAddr[MAX_ADDR_LEN];
	int         m_iFromLen;

	UDP_READ_OVERLAPPED(
		UINT8 *lpData,
		UINT32 ReqSize,
		UINT32*lpTransferredByets,
		SOCKADDR*lpOutFromAddr,
		int*        lpOutFromLen,
		LPVOID      lpParam,
		HANDLE      hEvent
		):
		SOCKET_OVERLAPPED(IO_READ)
	{
		m_lpData = lpData;
		m_ReqSize = ReqSize;
		m_lpTransferredByets = lpTransferredByets; 
		
		//返回给用户的信息。
		m_lpOutFromAddr = lpOutFromAddr;
		m_lpOutFromLen = lpOutFromLen;
		m_lpParam = lpParam;
		m_hEvent = hEvent;

		m_iFromLen = sizeof(m_FromAddr);
	}
};


struct UDP_WRITE_OVERLAPPED : SOCKET_OVERLAPPED
{
	UINT8 *	    m_lpData;
	UINT32      m_ReqSize;
	UINT32 *	m_lpTransferredByets;
	LPVOID      m_lpParam;
	HANDLE      m_hEvent;

	char		m_ToAddr[MAX_ADDR_LEN];
	int         m_iToLen;

	UDP_WRITE_OVERLAPPED(
		UINT8 *lpData,
		UINT32 ReqSize,
		UINT32*lpTransferredByets,
		const SOCKADDR*lpTo,
		int         iToLen,
		LPVOID      lpParam,
		HANDLE      hEvent
		) :
		SOCKET_OVERLAPPED(IO_WRITE)
	{
		m_lpData = lpData;
		m_ReqSize = ReqSize;
		m_lpTransferredByets = lpTransferredByets;

		memcpy(&m_ToAddr, lpTo, iToLen);
		m_iToLen = iToLen;

		m_lpParam = lpParam;
		m_hEvent = hEvent;
	}
};

class CUDPSocket :
	public CIOCPSocket
{
public:
	CUDPSocket();
	CUDPSocket(SOCKET hSocket);

	virtual ~CUDPSocket();

	//Asnyc operation callback.
	virtual void OnRecvFrom(
		BYTE * lpData,
		UINT32 nTransferredBytes,
		const SOCKADDR *lpFromAddr,
		int iFromLen,
		void * lpParam,
		DWORD Error){};
	
	virtual void OnSendTo(
		BYTE * lpData,
		UINT32 nTransferredBytes,
		const SOCKADDR *lpToAddr,
		int iToLen,
		void * lpParam,
		DWORD Error){};

	//
	BOOL Create()
	{
		BOOL bRet = FALSE;

		_write_lock(&m_rw_spinlock);

		if (m_flag & FLAG_CLOSED)
		{
			m_hSocket = socket(AF_INET, SOCK_DGRAM, 0);
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

private:
	void callback(SOCKET_OVERLAPPED *lpOverlapped, UINT32 nTransferredBytes, DWORD Error);
	void cb_readfrom(UDP_READ_OVERLAPPED *lpOverlapped, UINT32 nTransferredBytes, DWORD Error);
	void cb_sendto(UDP_WRITE_OVERLAPPED *lpOverlapped, UINT32 nTransferredBytes, DWORD Error);

public:
	//Async Recv and Send.
	BOOL RecvFrom(
		BYTE * lpData, 
		UINT32 Size, 
		UINT32 *lpTransferredBytes,
		SOCKADDR * lpFrom,
		PINT32 lpFromLen,
		void * lpParam = 0, 
		HANDLE hEvent = NULL );

	BOOL SendTo(
		BYTE * lpData, 
		UINT32 Size, 
		UINT32 *lpTransferredBytes,
		const SOCKADDR * lpTo, 
		INT32  iToLen, 

		void * lpParam = 0,
		HANDLE hEvent = NULL);
};

