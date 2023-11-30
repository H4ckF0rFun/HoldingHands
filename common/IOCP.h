#pragma once
#include <winsock2.h>
#include <MSWSock.h>
#include "list.h"
#include "spinlock.h"
#include <process.h>
#include <assert.h>

class CIOCPSocket;

#define __lock(cs)		EnterCriticalSection(&cs);
#define __unlock(cs)	LeaveCriticalSection(&cs);

struct thread_list
{
	struct list_node list;
	HANDLE  hThread;
	void    *lpParam;
};

struct event_list
{
	struct list_node list;
	HANDLE  hEvent;
};

#define IOCP_CREATING			    0x1
#define IOCP_CREATED				0x2
#define IOCP_CLOSING				0x4
#define IOCP_CLOSED					0x8


typedef int  (*typeInit)(void *lpParam);
typedef void (*typeFini)(void *lpParam);

class CIOCP
{
private:
	//flag 可以作为锁去使用.....

	volatile LONG		   m_state;
	volatile HANDLE		   m_hCompletionPort;							//完成端口句柄

	spinlock_t			   m_spinlock;
	//

	volatile ULONGLONG     m_ullUpload;
	volatile ULONGLONG     m_ullDownload;

	//
	volatile unsigned long m_BusyCount;
	volatile unsigned long m_MaxThreadCount;
	volatile unsigned long m_MaxConcurrent;
	volatile unsigned long m_WorkingCount;
	//
	volatile unsigned long m_ThreadCount;				
	
	list_node			   m_workers;
	list_node			   m_socks;
	
	//
	volatile unsigned long m_IoReqCount;					//总共未处理的Io请求个数.
	rw_spinlock_t          m_rw_spinlock;
	
	//IOCP worker thread initialize and fini function.
	typeInit               m_init;
	typeFini               m_fini;
	void *				   m_lpParam;
	
	//
	volatile unsigned long m_RefCount;
	virtual ~CIOCP();								//最终释放的时候必然没有引用了.
public:

	BOOL PostRecv(
		CIOCPSocket * sock,
		BYTE * lpData,
		UINT Size,
		LPOVERLAPPED lpOverlapped);

	BOOL PostSend(
		CIOCPSocket * sock,
		BYTE * lpData,
		UINT   Size,
		LPOVERLAPPED lpOverlapped);

	BOOL PostSendTo(
		CIOCPSocket * sock,
		BYTE * lpData,
		UINT Size,
		const SOCKADDR* lpTo,
		int iTolen,
		LPOVERLAPPED lpOverlapped);

	BOOL PostRecvFrom(
		CIOCPSocket * sock,
		BYTE * lpData,
		UINT Size,
		SOCKADDR* lpFrom,
		int* lpFromLen,
		LPOVERLAPPED lpOverlapped);

	BOOL PostAccept(
		CIOCPSocket * sock,
		SOCKET hClientSocket,
		BYTE   * AddrBuf,
		UINT   * lpRecvBytes,
		UINT  LocalAddrLength,
		UINT  RemoteAddrLength,
		LPOVERLAPPED lpOverlapped);

	BOOL PostConnect(
		CIOCPSocket * sock,
		const SOCKADDR* Addr,
		int iAddrLen,
		LPOVERLAPPED lpOverlapped);


private:
	void RemoveWorker(thread_list * node)
	{
		__spin_lock(m_spinlock);
		list_del_node(&m_WorkerList, &node->list);
		__spin_unlock(m_spinlock);

		CloseHandle(node->hThread);
		delete node;
	}

	typedef unsigned int (__stdcall * typeThreadProc)(void * ArgList);
	thread_list * AddWorker()
	{
		HANDLE hThread;
		thread_list * node = new thread_list;
		
		hThread = (HANDLE)_beginthreadex(NULL, 0, (typeThreadProc)CIOCP::Worker, node, CREATE_SUSPENDED, NULL);

		if (hThread == NULL)
		{
			delete node;
			return NULL;
		}

		node->hThread = hThread;
		node->lpParam = this;

		__spin_lock(m_spinlock);
		list_add_node(&m_workers, &node->list);
		__spin_unlock(m_spinlock);
		
		ResumeThread(hThread);
		return node;
	}

public:

	//if another object want to hold this object,he must call Get to inc refcount. 
	CIOCP * Get()			//thread-safe.
	{
		InterlockedIncrement(&m_RefCount);
		return this;
	}

	//
	void Put()				//thread-safe.
	{
		if (!InterlockedDecrement(&m_RefCount)){
			delete this; 
		}
	}

	void Close();			//thread-safe.

	static LPFN_CONNECTEX lpfnConnectEx;
	static LPFN_ACCEPTEX  lpfnAcceptEx;

	BOOL AssociateSock(CIOCPSocket * lpSock);			//thread-safe.
	void RemoveSock(CIOCPSocket*lpSock);				//thread-safe.
	BOOL Create();										//thread-safe.

	void GetTraffic(ULONGLONG traffic[2])				//thread-safe.
	{
		traffic[0] = m_ullDownload;
		traffic[1] = m_ullUpload;
	}

	static unsigned int __stdcall CIOCP::Worker(thread_list * list);
	CIOCP(UINT MaxThreadCount = 0, typeInit init = NULL, typeFini fini = NULL,void *lpParam = NULL);

};

