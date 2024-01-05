#pragma once

#include <stdio.h>
#include <Windows.h>


#define MAX_SIZE 0x10000

#define SEARCH_TRAVE_END			(WM_USER + 10086)				//子线程通知主线程
#define SEARCH_TRAVE_BEGIN			(WM_USER + 10087)				//主线程通知子线程
#define SEARCH_START_SEARCH_FILE	(WM_USER + 10088)				//通知主线程开始查找文件


typedef void(*pSearchCallFunc)(wchar_t* path, WIN32_FIND_DATAW* pfd,LPVOID param);
typedef void(*pSearchEndCall)(LPVOID Param);

class CSearchFile
{
	struct SEARCH_THREAD_CONTEXT
	{
		unsigned int				ThreadId;									//线程ID
		HANDLE						hThread;									//记录线程句柄
		CSearchFile*	pThis;										//保存对象指针
	};
	
	struct DEST_FILE_INFO
	{
		WCHAR FileName[4096];
		WCHAR StartDir[4096];
		DWORD FileAttrib;
	};
private:
	//-----------------------------------栈---------------------------
	LPWSTR*						m_DirStack;
	LPWSTR*						m_StackTop;
	DWORD						m_StackSize;
	void Push(WCHAR*pDir);
	LPWSTR Pop();
	//----------------------------------线程--------------------------
	DWORD						m_dwThreadCount;								//记录工作线程总数
	volatile long				m_lWorkingThreadCount;							//正在工作的工作者线程数量
	DWORD						m_dwMasterThreadId;								//主线程ID
	HANDLE						m_hMasterThread;
	//----------------------------------状态-------------------------
	volatile long				m_bStopSearching ;								//是否停止查找
	//----------------------------------其他-------------------------
	
	HANDLE						m_hInit;													
	HANDLE						m_hSearchEnd;									//查找结束

	//CRITICAL_SECTION			m_csStack;										//栈操作临界区
	volatile long				m_StackLock;									//自旋锁

	DEST_FILE_INFO				m_DestFile;										//目标文件信息
	
	pSearchCallFunc				m_pCallbakcAfterFind;							//查找到一个文件后将会调用此函数
	pSearchEndCall				m_pCallbakcAfterOver;							//结束之后调用这个函数
	LPVOID						m_Param;										//额外参数
	
	static void MasterThread(CSearchFile*pThis);					//主线程
	static unsigned int __stdcall WorkerThread(void*);		//工作线程
public:
	int Search(LPWSTR DestFileName, LPWSTR StartDir, DWORD FileAttribs,
		pSearchCallFunc pfun1,pSearchEndCall pfun2, LPVOID CallbackParam);		//查找文件

	void StopSearching();														//停止查找
	void WaitForTheEndOfSearch();												//等待查找结束
	BOOL IsSearching();															//是否正在查找
	CSearchFile(DWORD ThreadCount = 0);
	~CSearchFile();
};

