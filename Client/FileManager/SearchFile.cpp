#include "SearchFile.h"
#include <PROCESS.H>
#include "utils.h"

CSearchFile::~CSearchFile()
{
	//ֹͣ���ڽ��еĲ���
	StopSearching();
	//�ȴ����ҽ���
	WaitForTheEndOfSearch();
	//�˳�MasterThread
	PostThreadMessage(m_dwMasterThreadId, WM_QUIT, 0, 0);	//����WM_QUIT,Ȼ��GetMessage�᷵��0
	WaitForSingleObject(m_hMasterThread, INFINITE);			//�ȴ�MasterThread�˳�
	//printf("MasterThread:�˳�\n");
	//�ͷ�ջ
	if (m_DirStack)
	{
		free(m_DirStack);
		m_DirStack = NULL;
	}
	//ɾ���ٽ���
	//DeleteCriticalSection(&m_csStack);
	//�ر�Event
	CloseHandle(m_hInit);
	CloseHandle(m_hSearchEnd);
}

CSearchFile::CSearchFile(DWORD ThreadCount)
{
	//��ʼ��ջ
	m_StackSize = 0;
	m_DirStack = (LPTSTR*)malloc(sizeof(LPTSTR)* MAX_SIZE);
	m_StackTop = m_DirStack + MAX_SIZE;
	//�����߳����� CPU*2
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

	m_hSearchEnd = CreateEvent(NULL,TRUE,TRUE,NULL);	//�����������Ϊ�ֶ�,�еĵط���������ͨ��WaitForSingleObject���ж��¼�״̬�������������޸��¼�״̬,��ʼ״̬ΪTRUE(�����ҽ���)
	
	m_hInit = CreateEvent(NULL, FALSE, FALSE, NULL);
	//-------------------------------------------------------------------------------------------------------
	//�������߳�
	m_hMasterThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MasterThread, this, CREATE_SUSPENDED, &m_dwMasterThreadId);
	//��ʼ���ٽ���
	//InitializeCriticalSection(&m_csStack);
	m_StackLock = 0;

	//-------------------------------------------------------------------------------------------------------
	//�ָ��߳�ִ��
	SetThreadPriority(m_hMasterThread, THREAD_PRIORITY_LOWEST);
	ResumeThread(m_hMasterThread);
	//�ȴ����̳߳�ʼ�����
	WaitForSingleObject(m_hInit, INFINITE);
	//-------------------------------------------------------------------------------------------------------
	//����
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

	PeekMessage(NULL, NULL, NULL, NULL, PM_NOREMOVE);	//���ڴ�����Ϣ����
	//
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (msg.message != SEARCH_TRAVE_BEGIN) continue;

		if (msg.wParam == NULL)
			goto Notification;						   //�����̷߳�������

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
			//��Ϊ�ļ��� �� ���Ʋ�Ϊ..��.
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")))
			{

				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)			//���ļ�Ϊ��Ŀ¼,ѹ��ȫ��ջ��
				{
					NameLen = lstrlen(fd.cFileName);

					pTemp = (TCHAR*)malloc(sizeof(TCHAR) * (CurDirNameLen + 1 + NameLen + 1));

					lstrcpy(pTemp, pCurDir);
					*(pTemp + CurDirNameLen) = L'\\';
					lstrcpy(pTemp + CurDirNameLen + 1, fd.cFileName);
					
					pThis->Push(pTemp);
				}
				DEST_FILE_INFO*pDestFile = &pThis->m_DestFile;
				//�ҵ��ļ�
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
	//�������߳�
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

		SetThreadPriority(SearchThreadContext[i].hThread, THREAD_PRIORITY_HIGHEST);		//�������ȼ�
		ResumeThread(SearchThreadContext[i].hThread);									//�ָ�ִ��
	}

	//������Ϣ����
	PeekMessage(0, 0, 0, 0, 0);
	//��ʼ�����
	SetEvent(pThis->m_hInit);
	//-------------------------------------------------------------------------------------------------------------
	
	//�ȴ�Ͷ�ݿ�ʼ������Ϣ
	BOOL bPost;
	MSG msg;
	DWORD EndThreadCount = 0;				//��¼�����̵߳ĸ���
	
	while (GetMessage(&msg, NULL, NULL, NULL))
	{
		switch (msg.message)
		{
		case SEARCH_START_SEARCH_FILE:
			{
				 //��һ��Ŀ¼��ջ
				int StartDirLen = lstrlenW(pThis->m_DestFile.StartDir);
				LPWSTR StartDir = (WCHAR*)malloc(sizeof(WCHAR) * (StartDirLen + 1));

				lstrcpyW(StartDir, pThis->m_DestFile.StartDir);
				pThis->Push(StartDir);
				
				//��ʼ����ز���
				pThis->m_bStopSearching = FALSE;			//����Ƿ���ҽ���
				EndThreadCount = 0;

				//����������Ϊ0�Ļ��������̸߳����̷߳���һ������
				for (DWORD i = 0; i < pThis->m_dwThreadCount; i++)
					while(!PostThreadMessage(SearchThreadContext[i].ThreadId, SEARCH_TRAVE_BEGIN, 0, 0));
				//��ʼ������
				SetEvent(pThis->m_hInit);
			}
			break;
		case SEARCH_TRAVE_END:
			//ĳһ���̲߳��ҽ���(ֹͣ����Ӧ�õȴ�ĳһ�������߳��߳̽�����ǰ�Ĺ���)
			bPost = FALSE;
			//bPost�������������һ����if��������Ĵ����Ƿ�ִ��,���ִ�еĻ�����ô���߳�ִ����Ϻ󻹻��MasterThread Postһ��WM_END_TRAVEL��Ϣ
			//------------------------------------------------------------------------------------------------
			while (!pThis->m_bStopSearching && (pThis->m_StackSize > 0/*ջ�յ�ʱ����һ�ֿ����ǣ��̶߳���������������ǰΪֹ��û���ҵ�Ŀ¼*/ || pThis->m_lWorkingThreadCount > 0))
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
			//���ִ�е�����post��Ŀ���߳��Ѿ�ִ�����,ջ����WorkingThreadCount==0,���ǵ�ǰ�����߳��������̷߳�����һ��WM_End_Travel,���EndThreadCount���1��
			//�������һ�κ���һ��֮�������̵߳�����Stop,��ôEndThreadCountҲ����һ��
			if ((pThis->m_bStopSearching || 
				(pThis->m_StackSize == 0 && pThis->m_lWorkingThreadCount == 0))
				&& !bPost/*һ��Ҫ��֤ǰ��û��PostMessage*/)
			{
				++EndThreadCount;
				if (EndThreadCount == pThis->m_dwThreadCount)
				{
					//������ҽ��������ڵ����� Stop,��ôջ���ܲ�Ϊ�գ���Ҫ����ջ������
					if (pThis->m_bStopSearching)
					{
						while (pThis->m_StackSize > 0) 
							free(pThis->Pop());
					}
						
					if (pThis->m_pCallbakcAfterOver)
					{
						pThis->m_pCallbakcAfterOver(pThis->m_Param);
					}
					//���ҽ���
					SetEvent(pThis->m_hSearchEnd);
				}
			}
			break;
		default:
			continue;				//�޷�ʶ�����Ϣ
		}
	}
	//�ر�Workthreads
	for (i= 0; i < pThis->m_dwThreadCount; i++)
	{
		while(!PostThreadMessage(SearchThreadContext[i].ThreadId, WM_QUIT, 0, 0));
		WaitForSingleObject(SearchThreadContext[i].hThread, INFINITE);					//����Ӧ�����е����� 
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

//Search����ֻ����һ���߳��е���;
int CSearchFile::Search(LPWSTR DestFileName, LPWSTR StartDir, DWORD FileAttribs,
	pSearchCallFunc pfun1, pSearchEndCall pfun2, LPVOID CallFucnParam)
{
	//���ڲ����ļ�
	DWORD dwRet = WaitForSingleObject(m_hSearchEnd, 0);
	if (dwRet == WAIT_TIMEOUT)
		return -1;

	//��ʼ�����ļ�
	ResetEvent(m_hSearchEnd);
	//���ó�ʼ��Ϣ
	lstrcpyW(m_DestFile.FileName, DestFileName);
	lstrcpyW(m_DestFile.StartDir, StartDir);
	m_DestFile.FileAttrib = FileAttribs;
	
	m_pCallbakcAfterFind = pfun1;
	m_pCallbakcAfterOver = pfun2;
	m_Param = CallFucnParam;

	while(!PostThreadMessage(m_dwMasterThreadId, SEARCH_START_SEARCH_FILE, 0, 0));
	//�ȴ����ҳ�ʼ�����
	WaitForSingleObject(m_hInit,INFINITE);
	return 0;
}

void CSearchFile::StopSearching()
{
	//����Ѵ��������Ѿ����ҽ�����ֱ�ӷ���
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hSearchEnd, 0))
		return;
	//�ȴ����ҽ���
	InterlockedExchange(&m_bStopSearching, TRUE);
	WaitForTheEndOfSearch();
}
void CSearchFile::WaitForTheEndOfSearch()
{
	//���ڲ��ң�ֱ�ӷ���
	if (WAIT_TIMEOUT != WaitForSingleObject(m_hSearchEnd, 0))
		return;
	//�����ʱ��˵��δ������Ҳ�������ڲ��ң���ô�ٵȴ�
	WaitForSingleObject(m_hSearchEnd,INFINITE);
}

BOOL CSearchFile::IsSearching()
{
	BOOL bRet = FALSE;
	//��ʱ��˵��δ���������ڲ���
	if (WAIT_TIMEOUT == WaitForSingleObject(m_hSearchEnd, 0))
		bRet = TRUE;
	return bRet;
}