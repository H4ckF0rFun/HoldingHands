#include "stdafx.h"
#include "IOCP.h"
#include "IOCPSocket.h"
#include <stdio.h>
#include <assert.h>

#pragma comment(lib,"ws2_32")

LPFN_CONNECTEX CIOCP::lpfnConnectEx = NULL;
LPFN_ACCEPTEX	CIOCP::lpfnAcceptEx = NULL;

#define DIE(msg)     \
	do{					\
			MessageBoxA(NULL, msg, "die",MB_OK);	\
			ExitProcess(1);			\
	} while (0); \


CIOCP::CIOCP(UINT MaxThreadCount, typeInit init, typeFini fini,void *lpParam)
{
	SYSTEM_INFO si = { 0 };

	//Set thread counts;
	GetSystemInfo(&si);

	//最大并发数本机12个cpu ,24
	m_MaxConcurrent = si.dwNumberOfProcessors * 2;
	
	//
	m_MaxThreadCount = MaxThreadCount;
	if (!m_MaxThreadCount)
	{
		m_MaxThreadCount = m_MaxConcurrent + 4;
	}

	//
	m_BusyCount = 0;

	//thread worker initialize and finialize funtions.
	m_init = init;
	m_fini = fini;
	m_lpParam = lpParam; 

	//initialize worklist.
	m_ThreadCount = 0;
	m_WorkingCount = 0;

	//initialize spinlock.
	InterlockedExchange(&m_spinlock, 0);

	list_init(&m_workers);

	//initialize socklist
	list_init(&m_socks);


	//closed.
	m_state = IOCP_CLOSED;

	//Create completion port
	m_hCompletionPort = NULL;
	//
	m_IoReqCount = 0;
	_rw_spinlock_init(&m_rw_spinlock);

	//traffic
	m_ullUpload = 0;
	m_ullDownload = 0;

	//ref
	m_RefCount = 1;

}


CIOCP::~CIOCP()
{
	assert(m_state == IOCP_CLOSED);
	dbg_log("CIOCP::~CIOCP()");
}

void CIOCP::RemoveSock(CIOCPSocket*lpSock)
{
	__spin_lock(m_spinlock);
	list_del_node(&m_socks, &lpSock->m_list);
	__spin_unlock(m_spinlock);

	lpSock->m_Iocp = NULL;
	lpSock->Put();
}


BOOL CIOCP::AssociateSock(CIOCPSocket * lpSock)
{
	SOCKET hSock;
	BOOL   bRet = FALSE;

	_read_lock(&m_rw_spinlock);

	if (m_state & (IOCP_CLOSING | IOCP_CLOSED))
		goto __failed__;

	//check sock is closed?
	_write_lock(&lpSock->m_rw_spinlock);
	lpSock->Get();

	if (lpSock->m_flag & (FLAG_CLOSING | FLAG_CLOSED | FLAG_ASSOCIATED))
		goto __failed_0;

	hSock = lpSock->m_hSocket;

	if (m_hCompletionPort != CreateIoCompletionPort(
		(HANDLE)hSock,
		m_hCompletionPort,
		(ULONG_PTR)lpSock,
		NULL))
	{
		goto __failed_0;
	}

	__spin_lock(m_spinlock);
	list_add_node(&m_socks, &lpSock->m_list);
	__spin_unlock(m_spinlock);

	lpSock->m_Iocp = this;
	lpSock->m_flag |= FLAG_ASSOCIATED;

	bRet = TRUE;

__failed_0:
	_write_unlock(&lpSock->m_rw_spinlock);

__failed__:

	if (!bRet)
	{
		lpSock->Put();
	}

	_read_unlock(&m_rw_spinlock);
	return bRet;
}

BOOL CIOCP::PostRecv(
	CIOCPSocket * sock,
	BYTE * lpData,
	UINT   Size,
	LPOVERLAPPED lpOverlapped)
{
	//Post Recv
	WSABUF			wsabuf;
	INT32			nRet;
	DWORD			flag = 0;
	BOOL            retval = FALSE;
	SOCKET          hSock;

	wsabuf.buf = (CHAR*)lpData;
	wsabuf.len = Size;

	_read_lock(&m_rw_spinlock);

	if (m_state & (IOCP_CLOSING | IOCP_CLOSED))
		goto __failed__;

	sock->Get();
	hSock = sock->m_hSocket;

	nRet = WSARecv(
		hSock,
		&wsabuf,
		1,
		NULL,
		&flag,
		lpOverlapped,
		NULL
		);

	if (INVALID_SOCKET == nRet && WSAGetLastError() != WSA_IO_PENDING)
	{
		sock->Put();
		goto __failed__;
	}

	InterlockedIncrement(&m_IoReqCount);
	retval = TRUE;
__failed__:
	_read_unlock(&m_rw_spinlock);
	return retval;
}

BOOL CIOCP::PostSend(
	CIOCPSocket * sock,
	BYTE * lpData,
	UINT Size,
	LPOVERLAPPED lpOverlapped)
{
	//Post Recv
	WSABUF			wsabuf;
	INT32			nRet;
	DWORD			flag = 0;
	BOOL            retval = FALSE;
	SOCKET			hSock;

	wsabuf.buf = (CHAR*)lpData;
	wsabuf.len = Size;

	_read_lock(&m_rw_spinlock);

	if (m_state & (IOCP_CLOSING | IOCP_CLOSED))
		goto __failed__;

	sock->Get();
	hSock = sock->m_hSocket;


	nRet = WSASend(
		hSock,
		&wsabuf,
		1,
		NULL,
		flag,
		lpOverlapped,
		NULL
		);

	
	if (INVALID_SOCKET == nRet && WSAGetLastError() != WSA_IO_PENDING)
	{
		sock->Put();
		goto __failed__;
	}

	InterlockedIncrement(&m_IoReqCount);
	retval = TRUE;
__failed__:
	_read_unlock(&m_rw_spinlock);
	return retval;
}


BOOL CIOCP::PostSendTo(
	CIOCPSocket * sock,
	BYTE * lpData,
	UINT Size,
	const SOCKADDR* lpTo,
	int iTolen,
	LPOVERLAPPED lpOverlapped)
{
	//Post Recv
	WSABUF			wsabuf;
	INT32			nRet;
	DWORD			flag = 0;
	BOOL            retval = FALSE;
	SOCKET			hSock;
	
	wsabuf.buf = (CHAR*)lpData;
	wsabuf.len = Size;
	
	_read_lock(&m_rw_spinlock);

	if (m_state & (IOCP_CLOSING | IOCP_CLOSED))
		goto __failed__;

	sock->Get();
	hSock = sock->m_hSocket;


	nRet = WSASendTo(
		hSock,
		&wsabuf,
		1,
		NULL,
		flag,
		lpTo,
		iTolen,
		lpOverlapped,
		NULL
		);


	if (INVALID_SOCKET == nRet && WSAGetLastError() != WSA_IO_PENDING)
	{
		sock->Put();
		goto __failed__;
	}

	InterlockedIncrement(&m_IoReqCount);
	retval = TRUE;
__failed__:
	_read_unlock(&m_rw_spinlock);
	return retval;
}

BOOL CIOCP::PostRecvFrom(
	CIOCPSocket * sock,
	BYTE * lpData,
	UINT Size,
	SOCKADDR* lpFrom,
	int* lpFromLen,
	LPOVERLAPPED lpOverlapped)
{
	//Post Recv
	WSABUF			wsabuf;
	INT32			nRet;
	DWORD			flag = 0;
	BOOL            retval = FALSE;
	SOCKET			hSock;

	wsabuf.buf = (CHAR*)lpData;
	wsabuf.len = Size;

	_read_lock(&m_rw_spinlock);

	if (m_state & (IOCP_CLOSING | IOCP_CLOSED))
		goto __failed__;

	sock->Get();

	hSock = sock->m_hSocket;


	nRet = WSARecvFrom(
		hSock,
		&wsabuf,
		1,
		NULL,
		&flag,
		lpFrom,
		lpFromLen,
		lpOverlapped,
		NULL
		);


	if (INVALID_SOCKET == nRet && WSAGetLastError() != WSA_IO_PENDING)
	{
		sock->Put();
		goto __failed__;
	}

	InterlockedIncrement(&m_IoReqCount);
	retval = TRUE;
__failed__:
	_read_unlock(&m_rw_spinlock);
	return retval;
}

BOOL CIOCP::PostAccept(
	CIOCPSocket * sock,
	SOCKET hClientSocket,
	BYTE   * AddrBuf,
	UINT   * lpRecvBytes,
	UINT  LocalAddrLength,
	UINT  RemoteAddrLength,
	LPOVERLAPPED lpOverlapped)
{
	INT				iResult = 0;
	GUID			GuidAcceptEx = WSAID_ACCEPTEX;
	SOCKET			temp;
	DWORD			dwBytes;
	INT32			nRet;
	BOOL            retval = FALSE;
	SOCKET			hSock;
	
	_read_lock(&m_rw_spinlock);

	if (m_state & (IOCP_CLOSING | IOCP_CLOSED))
		goto __failed__;

	if (!lpfnAcceptEx)
	{
		iResult = INVALID_SOCKET;
		temp    = socket(AF_INET, SOCK_STREAM, NULL);
		
		if (temp != INVALID_SOCKET){
			iResult = WSAIoctl(
				temp,
				SIO_GET_EXTENSION_FUNCTION_POINTER, 
				&GuidAcceptEx, sizeof(GuidAcceptEx),
				&lpfnAcceptEx, sizeof(lpfnAcceptEx),
				&dwBytes,
				NULL, 
				NULL);

			closesocket(temp);
		}
	}

	if (lpfnAcceptEx == NULL)
		goto __failed__;

	sock->Get();
	hSock = sock->m_hSocket;


	nRet = lpfnAcceptEx(
		hSock,
		hClientSocket,
		AddrBuf,
		0,
		LocalAddrLength,
		RemoteAddrLength,
		(LPDWORD)lpRecvBytes,
		lpOverlapped);

	if (!nRet && WSAGetLastError() != WSA_IO_PENDING)
	{
		sock->Put();
		goto __failed__;
	}

	InterlockedIncrement(&m_IoReqCount);
	retval = TRUE;

__failed__:
	_read_unlock(&m_rw_spinlock);
	return retval;
}

BOOL CIOCP::PostConnect(
	CIOCPSocket * sock,
	const SOCKADDR* Addr,
	int iAddrLen,
	LPOVERLAPPED lpOverlapped)
{
	INT				iResult = 0;
	GUID			GuidConnectEx = WSAID_CONNECTEX;
	SOCKET			temp;
	DWORD			dwBytes;
	SOCKET          hSock;
	INT32			nRet;
	BOOL            retval = FALSE;
	
	_read_lock(&m_rw_spinlock);

	if (m_state & (IOCP_CLOSING | IOCP_CLOSED))
		goto __failed__;

	if (!lpfnConnectEx)
	{
		iResult = INVALID_SOCKET;
		temp = socket(AF_INET, SOCK_STREAM, NULL);

		if (temp != INVALID_SOCKET)
		{
			iResult = WSAIoctl(
				temp,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidConnectEx, sizeof(GuidConnectEx),
				&lpfnConnectEx, sizeof(lpfnConnectEx),
				&dwBytes,
				NULL,
				NULL);

			closesocket(temp);
		}
	}

	if (lpfnConnectEx == NULL)
		goto __failed__;

	
	sock->Get();
	hSock = sock->m_hSocket;


	nRet = lpfnConnectEx(
		hSock,
		(SOCKADDR*)Addr,
		iAddrLen,
		NULL,
		0,
		NULL,
		lpOverlapped
		);

	if (!nRet && WSAGetLastError() != WSA_IO_PENDING)
	{
		sock->Put();
		goto __failed__;
	}

	InterlockedIncrement(&m_IoReqCount);
	retval = TRUE;

__failed__:
	_read_unlock(&m_rw_spinlock);
	return retval;
}

unsigned int CIOCP::Worker(thread_list * list)
{
	CIOCP*			Iocp  = (CIOCP*)list->lpParam;
	BOOL			bResult;
	DWORD			nTransferredBytes;
	SOCKET_OVERLAPPED  *   lpOverlappedplus;
	DWORD           dwBusyCount;
	DWORD			Error  = 0;
	CIOCPSocket  *  sock   = NULL;
	HANDLE          hCompletionPort = NULL;
	BOOL			loop = TRUE;
	ULONGLONG		leftIoReqCnt;

	if (Iocp->m_init)
		assert(!Iocp->m_init(Iocp->m_lpParam));

	while (loop)
	{
		bResult = FALSE;
		nTransferredBytes = 0;
		lpOverlappedplus = NULL;
		Error = 0;

		//wait completion status.
		bResult = GetQueuedCompletionStatus(
			Iocp->m_hCompletionPort,
			&nTransferredBytes,
			(PULONG_PTR)&sock,
			(LPOVERLAPPED*)&lpOverlappedplus,
			INFINITE);

		//manual post.
		if (sock == (CIOCPSocket*)INVALID_SOCKET)	//thread is active by post ....
		{
			if (!Iocp->m_IoReqCount)
			{
				loop = FALSE;

				//wake up next worker.
				PostQueuedCompletionStatus(Iocp->m_hCompletionPort,
					0,
					(ULONG_PTR)INVALID_SOCKET,
					NULL);
			}
			continue;
		}
		
		//skip
		if (!sock)
			continue;

		if (!bResult)
			Error = WSAGetLastError();
			
		/*
			处理IO_READ,IO_WRITE;
			此处需要。简单的线程调度,若nBusyCount == ThreadCount,那么就创建一个新的线程
			(为了防止所有线程都挂起而导致writeIOMsg无法被处理,
			emmm可能这个情况发生的几率很小) 只要线程处于挂起(SendMessage,SuspenThread
			,WaitForSingleObject等时), 新创建的线程才会被IOCP完成端口使用.
			否则新的线程没什么作用,始终处于挂起状态原因是为
			了保证最大并发数量小于 初始设定值。
		*/

		//create new worker thread if all worker is used now.
		if ((1 + InterlockedIncrement(&Iocp->m_BusyCount))
			< InterlockedIncrement(&Iocp->m_WorkingCount))
		{
			InterlockedDecrement(&Iocp->m_WorkingCount);
		}
		else {
			Iocp->Get();
			InterlockedIncrement(&Iocp->m_ThreadCount);
			if (!Iocp->AddWorker()){
				InterlockedDecrement(&Iocp->m_WorkingCount);
				InterlockedIncrement(&Iocp->m_ThreadCount);
				Iocp->Put();
			}
		}
		
		/***********Update the total amount of data uploaded and downloaded ***********/
		if (nTransferredBytes)
		{
			switch (lpOverlappedplus->m_IOtype)
			{
			case IO_READ:
				InterlockedExchangeAdd(&Iocp->m_ullDownload, nTransferredBytes);
				break;

			case IO_WRITE:
				InterlockedExchangeAdd(&Iocp->m_ullUpload, nTransferredBytes);
				break;
			}
		}
		/*******************************************************/
		
		sock->callback(lpOverlappedplus, nTransferredBytes, Error);
		sock->Put();		//don't use sock any more.			
		
		leftIoReqCnt = InterlockedDecrement(&Iocp->m_IoReqCount);

		_read_lock(&Iocp->m_rw_spinlock);
		if (Iocp->m_state & IOCP_CLOSING)
		{
			if (!leftIoReqCnt)
			{
				loop = FALSE;
				
				//wake up next worker.
				PostQueuedCompletionStatus(Iocp->m_hCompletionPort,
					0,
					(ULONG_PTR)INVALID_SOCKET,
					NULL);
			}
		}
		_read_unlock(&Iocp->m_rw_spinlock);


		/*******************************************************/
		dwBusyCount = InterlockedDecrement(&Iocp->m_BusyCount);
		
		if ((InterlockedDecrement(&Iocp->m_WorkingCount) > Iocp->m_MaxThreadCount)
			&& (dwBusyCount < (1 + Iocp->m_MaxThreadCount / 2)))
		{	
			loop = FALSE;
		}
		else{
			InterlockedIncrement(&Iocp->m_WorkingCount);
		}
	}

	//remove current thread from worker list.
	Iocp->RemoveWorker(list);

	//it is closing ? 
	_write_lock(&Iocp->m_rw_spinlock);
	if (!InterlockedDecrement(&Iocp->m_ThreadCount)
		&& Iocp->m_state & IOCP_CLOSING)
	{
		//Close completion port.
		hCompletionPort = Iocp->m_hCompletionPort;
		Iocp->m_hCompletionPort = NULL;
		Iocp->m_state = IOCP_CLOSED;				//正真的关闭完成.
	}
	_write_unlock(&Iocp->m_rw_spinlock);

	if (hCompletionPort)
		CloseHandle(hCompletionPort);

	if (Iocp->m_fini)
		Iocp->m_fini(Iocp->m_lpParam);

	//release iocp.
	Iocp->Put();
	return 0;
}


BOOL CIOCP::Create()
{
	BOOL bRet = FALSE;

	_read_lock(&m_rw_spinlock);
	if (m_state & (IOCP_CREATING | IOCP_CREATED))
	{
		_read_unlock(&m_rw_spinlock);
		return TRUE; 
	}
	_read_unlock(&m_rw_spinlock);


	_write_lock (&m_rw_spinlock);
	if (m_state & (IOCP_CREATING | IOCP_CREATED))
	{
		_write_unlock(&m_rw_spinlock);
		return TRUE;
	}

	if (m_state & IOCP_CLOSING)
	{
		_write_unlock(&m_rw_spinlock);
		return FALSE;
	}
	
	//m_state must be closed.

	m_state = IOCP_CREATING;
	_write_unlock(&m_rw_spinlock);

	//Create completion port
	m_hCompletionPort = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,
		NULL,
		NULL,
		m_MaxConcurrent);

	if (NULL == m_hCompletionPort)
	{
		goto __failed__;
	}

	for (int i = 0; i < m_MaxThreadCount; i++)
	{
		InterlockedIncrement(&m_ThreadCount);
		Get();

		if (!AddWorker()){
			InterlockedDecrement(&m_ThreadCount);
			Put();
		}
	}
		
	bRet = TRUE;
	m_state = IOCP_CREATED;

	//
__failed__:
	if (!bRet)			//creating ,but create failed;
	{
		if (m_hCompletionPort)
		{
			CloseHandle(m_hCompletionPort);
			m_hCompletionPort = NULL;
		}
	}
	return bRet;
}

/*
	一旦 Post 成功,那么就一定要处理掉sock的 IO请求。因为要释放
	Overlapped以及其他的数据.
*/

void CIOCP::Close()
{
	list_node *   p;
	_read_lock(&m_rw_spinlock);
	if (m_state != IOCP_CREATED)	//may be closing or creating....
	{
		_read_unlock(&m_rw_spinlock);
		return;
	}
	_read_unlock(&m_rw_spinlock);
	
	_write_lock(&m_rw_spinlock);
	if (m_state != IOCP_CREATED)	//may be closing or creating....
	{
		_write_unlock(&m_rw_spinlock);
		return;
	}

	m_state = IOCP_CLOSING;
	_write_unlock(&m_rw_spinlock);
	
	//Now, No io request will be posted successfully.and there 
	//will no new sock to be associated with this iocp.

	//Cancel All Io request of associated socks,otherwise,
	//the overlapped allocated couludn't be free.

__CancelIO:
	__spin_lock(m_spinlock);
	p = m_socks.next;

	while (p != &m_socks)
	{
		ULONG         off  = (ULONG)&((CIOCPSocket*)NULL)->m_list;
		CIOCPSocket * sock = (CIOCPSocket*)(((ULONG)p) - off);
		BOOL          bRemove = FALSE;

		_write_lock(&sock->m_rw_spinlock);
		if (sock->m_flag & FLAG_ASSOCIATED)
		{
			sock->m_flag &= (~FLAG_ASSOCIATED);
			bRemove = TRUE;
		}
		_write_unlock(&sock->m_rw_spinlock);

		if (bRemove)
		{
			list_del_node(m_socks, p);
			__spin_unlock(m_spinlock);

			//after we set FLAG_ASSOCIATED,no io request will be posted.
			//now ,we need cancel all io reqeusts be posted previously.

			_read_lock(&sock->m_rw_spinlock);
			if (!(sock->m_flag & (FLAG_CLOSING | FLAG_CLOSED)))
			{
				//cancel io,then GetQueuedCompletionStatus will get a canceled statu and process it.
				CancelIoEx((HANDLE)sock->m_hSocket, NULL);
			}
			_read_unlock(&sock->m_rw_spinlock);
			//release the sock.
			sock->Put();
			goto __CancelIO;
		}
		else{
			p = p->next; 
		}

		/*
		else:
			//this sock has been remove from iocp,
			//it will be remove from list by itself.
			//do nothing....
		*/
	}
	__spin_unlock(m_spinlock);

	//Now there will no new thread in queue.
	//only wake up one thread.

	/*
		1. 所有线程都在等待,队列中没有可以取出的完成事件
		2. 部分线程在等待,队列中只有部分.
		3. 部分线程在等待,队列中有大于等待线程数的.
		4. 所有线程都在忙,队列中没有了
		5. 所有线程都在忙,队列中还有.
	*/

	PostQueuedCompletionStatus(
		m_hCompletionPort, 
		0, 
		(ULONG_PTR)INVALID_SOCKET, 
		NULL);

	return;
}
