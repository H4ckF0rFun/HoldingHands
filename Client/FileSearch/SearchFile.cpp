#include "SearchFile.h"
#include <PROCESS.H>
#include "utils.h"

CSearchFile::~CSearchFile()
{
	//停止正在进行的查找
	StopSearching();
	//等待查找结束
	WaitForTheEndOfSearch();
	//退出MasterThread
	PostThreadMessage(m_dwMasterThreadId, WM_QUIT, 0, 0);	//发送WM_QUIT,然后GetMessage会返回0
	WaitForSingleObject(m_hMasterThread, INFINITE);			//等待MasterThread退出
	//printf("MasterThread:退出\n");
	//释放栈
	if (m_DirStack)
	{
		free(m_DirStack);
		m_DirStack = NULL;
	}
	//删除临界区
	//DeleteCriticalSection(&m_csStack);
	//关闭Event
	CloseHandle(m_hInit);
	CloseHandle(m_hSearchEnd);
}

CSearchFile::CSearchFile(DWORD ThreadCount)
{
	//初始化栈
	m_StackSize = 0;
	m_DirStack = (LPTSTR*)malloc(sizeof(LPTSTR)* MAX_SIZE);
	m_StackTop = m_DirStack + MAX_SIZE;
	//设置线程数量 CPU*2
	if (ThreadCount == 0)
	{
		SYSTEM_INFO SystemInfo = { 0 };
		GetSystemInfo(&SystemInfo);
		ThreadCount = SystemInfo.dwNumberOfProcessors * 2;
	}
	m_dwThreadCount = ThreadCount;

	m_lWorkingThreadCount = 0;
	
	m_bStopSearching = FALSE;


	m_pCallbakcAfterFind = NULL;
	m_pCallbakcAfterOver = NULL;

	m_hSearchEnd = CreateEvent(NULL,TRUE,TRUE,NULL);	//这个必须设置为手动,有的地方仅仅是想通过WaitForSingleObject来判断事件状态，而并不是想修改事件状态,初始状态为TRUE(即查找结束)
	
	m_hInit = CreateEvent(NULL, FALSE, FALSE, NULL);
	//-------------------------------------------------------------------------------------------------------
	//创建主线程
	m_hMasterThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MasterThread, this, CREATE_SUSPENDED, &m_dwMasterThreadId);
	//初始化临界区
	//InitializeCriticalSection(&m_csStack);
	m_StackLock = 0;

	//-------------------------------------------------------------------------------------------------------
	//恢复线程执行
	SetThreadPriority(m_hMasterThread, THREAD_PRIORITY_LOWEST);
	ResumeThread(m_hMasterThread);
	//等待主线程初始化完毕
	WaitForSingleObject(m_hInit, INFINITE);
	//-------------------------------------------------------------------------------------------------------
	//返回
	return;
}


unsigned int __stdcall CSearchFile::WorkerThread(void*Param)
{
	//
	SEARCH_THREAD_CONTEXT*pSearchThreadContext = (SEARCH_THREAD_CONTEXT*)Param;
	MSG msg;
	WCHAR*	pCurDir = NULL, *pFirstFileName = NULL, *pTemp = NULL;
	DWORD	CurDirNameLen = 0, NameLen = 0;
	BOOL	bFindNextRet = NULL;
	HANDLE	hFirst = NULL;
	WIN32_FIND_DATAW fd;
	CSearchFile *pThis = pSearchThreadContext->pThis;

	PeekMessage(NULL, NULL, NULL, NULL, PM_NOREMOVE);	//用于创建消息队列
	//
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (msg.message != SEARCH_TRAVE_BEGIN) continue;

		if (msg.wParam == NULL)
			goto Notification;						   //给主线程发送请求

		pCurDir = (TCHAR*)msg.wParam;
		CurDirNameLen = lstrlen(pCurDir);
		pFirstFileName = (TCHAR*)malloc(sizeof(TCHAR)* (CurDirNameLen + 3));

		lstrcpyW(pFirstFileName, pCurDir);

		*(pFirstFileName + CurDirNameLen) = L'\\';
		*(pFirstFileName + CurDirNameLen + 1) = L'*';
		*(pFirstFileName + CurDirNameLen + 2) = 0;

		//Begin trave dir;
	
		hFirst = FindFirstFile(pFirstFileName, &fd);
		bFindNextRet = TRUE;

		while (hFirst != INVALID_HANDLE_VALUE && bFindNextRet)
		{
			//不为文件夹 或 名称不为..和.
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")))
			{

				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)			//该文件为子目录,压入全局栈中
				{
					NameLen = lstrlen(fd.cFileName);

					pTemp = (TCHAR*)malloc(sizeof(TCHAR) * (CurDirNameLen + 1 + NameLen + 1));

					lstrcpy(pTemp, pCurDir);
					*(pTemp + CurDirNameLen) = L'\\';
					lstrcpy(pTemp + CurDirNameLen + 1, fd.cFileName);
					
					pThis->Push(pTemp);
				}
				DEST_FILE_INFO*pDestFile = &pThis->m_DestFile;
				//找到文件
				if (StrStr(fd.cFileName, pDestFile->FileName) && 
					(!pDestFile->FileAttrib ||((pDestFile->FileAttrib&fd.dwFileAttributes) == pDestFile->FileAttrib)))
				{
					if (pThis->m_pCallbakcAfterFind)
					{
						pThis->m_pCallbakcAfterFind(pCurDir, &fd, pThis->m_Param);
					}
				}
			}
			bFindNextRet = FindNextFile(hFirst, &fd);
		}
		if (INVALID_HANDLE_VALUE != hFirst)	FindClose(hFirst);

		free(pCurDir);
		free(pFirstFileName);

		InterlockedDecrement(&pThis->m_lWorkingThreadCount);

	Notification:
		PostThreadMessage(pThis->m_dwMasterThreadId, SEARCH_TRAVE_END, (WPARAM)pSearchThreadContext, 0);
	}

	_endthreadex(0);
	return 0;
}

void CSearchFile::MasterThread(CSearchFile*pThis)
{
	DWORD i=0;
	//-------------------------------------------------------------------------------------------------------------
	//创建子线程
	SEARCH_THREAD_CONTEXT *SearchThreadContext = (SEARCH_THREAD_CONTEXT*)malloc(sizeof(SEARCH_THREAD_CONTEXT) * pThis->m_dwThreadCount);
	
	for (i = 0; i < pThis->m_dwThreadCount; i++)
	{
		SearchThreadContext[i].pThis = pThis;
		SearchThreadContext[i].hThread = (HANDLE)_beginthreadex(0,
			0, 
			CSearchFile::WorkerThread,
			(LPVOID)(SearchThreadContext + i), 
			CREATE_SUSPENDED, 
			&SearchThreadContext[i].ThreadId);

		SetThreadPriority(SearchThreadContext[i].hThread, THREAD_PRIORITY_HIGHEST);		//设置优先级
		ResumeThread(SearchThreadContext[i].hThread);									//恢复执行
	}

	//创建消息队列
	PeekMessage(0, 0, 0, 0, 0);
	//初始化完毕
	SetEvent(pThis->m_hInit);
	//-------------------------------------------------------------------------------------------------------------
	
	//等待投递开始查找消息
	BOOL bPost;
	MSG msg;
	DWORD EndThreadCount = 0;				//记录结束线程的个数
	
	while (GetMessage(&msg, NULL, NULL, NULL))
	{
		switch (msg.message)
		{
		case SEARCH_START_SEARCH_FILE:
			{
				 //第一个目录入栈
				int StartDirLen = lstrlenW(pThis->m_DestFile.StartDir);
				LPWSTR StartDir = (WCHAR*)malloc(sizeof(WCHAR) * (StartDirLen + 1));

				lstrcpyW(StartDir, pThis->m_DestFile.StartDir);
				pThis->Push(StartDir);
				
				//初始化相关参数
				pThis->m_bStopSearching = FALSE;			//标记是否查找结束
				EndThreadCount = 0;

				//后两个参数为0的话，工作线程给主线程发送一个请求
				for (DWORD i = 0; i < pThis->m_dwThreadCount; i++)
					while(!PostThreadMessage(SearchThreadContext[i].ThreadId, SEARCH_TRAVE_BEGIN, 0, 0));
				//初始化结束
				SetEvent(pThis->m_hInit);
			}
			break;
		case SEARCH_TRAVE_END:
			//某一个线程查找结束(停止查找应该等待某一个工作线程线程结束当前的工作)
			bPost = FALSE;
			//bPost用来标记下面这一段中if成立情况的代码是否执行,如果执行的话，那么该线程执行完毕后还会给MasterThread Post一个WM_END_TRAVEL消息
			//------------------------------------------------------------------------------------------------
			while (!pThis->m_bStopSearching && (pThis->m_StackSize > 0/*栈空的时候有一种可能是：线程都在搜索，但到当前为止还没有找到目录*/ || pThis->m_lWorkingThreadCount > 0))
			{
				if (pThis->m_StackSize > 0)
				{
					bPost = TRUE;
					InterlockedIncrement(&pThis->m_lWorkingThreadCount);
					while(!PostThreadMessage(((SEARCH_THREAD_CONTEXT*)(msg.wParam))->ThreadId, SEARCH_TRAVE_BEGIN, (WPARAM)pThis->Pop(), 0));
					break;
				}
				Sleep(1);
			}
			//------------------------------------------------------------------------------------------------
			//如果执行到这里post的目标线程已经执行完毕,栈空且WorkingThreadCount==0,但是当前工作线程又向主线程发送了一个WM_End_Travel,最后EndThreadCount会大1）
			//如果在上一段和这一段之间其他线程调用了Stop,那么EndThreadCount也会多加一次
			if ((pThis->m_bStopSearching || 
				(pThis->m_StackSize == 0 && pThis->m_lWorkingThreadCount == 0))
				&& !bPost/*一定要保证前面没有PostMessage*/)
			{
				++EndThreadCount;
				if (EndThreadCount == pThis->m_dwThreadCount)
				{
					//如果查找结束是由于调用了 Stop,那么栈可能不为空，需要清理栈内数据
					if (pThis->m_bStopSearching)
					{
						while (pThis->m_StackSize > 0) 
							free(pThis->Pop());
					}
						
					if (pThis->m_pCallbakcAfterOver)
					{
						pThis->m_pCallbakcAfterOver(pThis->m_Param);
					}
					//查找结束
					SetEvent(pThis->m_hSearchEnd);
				}
			}
			break;
		default:
			continue;				//无法识别的消息
		}
	}
	//关闭Workthreads
	for (i= 0; i < pThis->m_dwThreadCount; i++)
	{
		while(!PostThreadMessage(SearchThreadContext[i].ThreadId, WM_QUIT, 0, 0));
		WaitForSingleObject(SearchThreadContext[i].hThread, INFINITE);					//这行应该是有点多余的 
		CloseHandle(SearchThreadContext[i].hThread);
	}

	free(SearchThreadContext);
}


void CSearchFile::Push(WCHAR*pDir)
{
	do 
	{
		_asm
		{
			pause
		}
	} while (InterlockedExchange(&m_StackLock,1));

	++m_StackSize;
	--m_StackTop;
	*m_StackTop = pDir;
	
	
	InterlockedExchange(&m_StackLock,0);
}

LPWSTR CSearchFile::Pop()
{
	LPWSTR ret;
	
	//lock 
	do 
	{
		_asm{
			pause
		}
	} while (InterlockedExchange(&m_StackLock,1));

	--m_StackSize;
	++m_StackTop;
	ret = *(m_StackTop - 1);
	//unlock
	InterlockedExchange(&m_StackLock,0);
	
	return ret;
}

//Search函数只能在一个线程中调用;
int CSearchFile::Search(LPWSTR DestFileName, LPWSTR StartDir, DWORD FileAttribs,
	pSearchCallFunc pfun1, pSearchEndCall pfun2, LPVOID CallFucnParam)
{
	//正在查找文件
	DWORD dwRet = WaitForSingleObject(m_hSearchEnd, 0);
	if (dwRet == WAIT_TIMEOUT)
		return -1;

	//开始查找文件
	ResetEvent(m_hSearchEnd);
	//设置初始信息
	lstrcpyW(m_DestFile.FileName, DestFileName);
	lstrcpyW(m_DestFile.StartDir, StartDir);
	m_DestFile.FileAttrib = FileAttribs;
	
	m_pCallbakcAfterFind = pfun1;
	m_pCallbakcAfterOver = pfun2;
	m_Param = CallFucnParam;

	while(!PostThreadMessage(m_dwMasterThreadId, SEARCH_START_SEARCH_FILE, 0, 0));
	//等待查找初始化完毕
	WaitForSingleObject(m_hInit,INFINITE);
	return 0;
}

void CSearchFile::StopSearching()
{
	//如果已触发，即已经查找结束，直接返回
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hSearchEnd, 0))
		return;
	//等待查找结束
	InterlockedExchange(&m_bStopSearching, TRUE);
	WaitForTheEndOfSearch();
}
void CSearchFile::WaitForTheEndOfSearch()
{
	//不在查找，直接返回
	if (WAIT_TIMEOUT != WaitForSingleObject(m_hSearchEnd, 0))
		return;
	//如果超时，说明未触发，也就是正在查找，那么再等待
	WaitForSingleObject(m_hSearchEnd,INFINITE);
}

BOOL CSearchFile::IsSearching()
{
	BOOL bRet = FALSE;
	//超时，说明未触发，正在查找
	if (WAIT_TIMEOUT == WaitForSingleObject(m_hSearchEnd, 0))
		bRet = TRUE;
	return bRet;
}