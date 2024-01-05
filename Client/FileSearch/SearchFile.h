#pragma once

#include <stdio.h>
#include <Windows.h>


#define MAX_SIZE 0x10000

#define SEARCH_TRAVE_END			(WM_USER + 10086)				//���߳�֪ͨ���߳�
#define SEARCH_TRAVE_BEGIN			(WM_USER + 10087)				//���߳�֪ͨ���߳�
#define SEARCH_START_SEARCH_FILE	(WM_USER + 10088)				//֪ͨ���߳̿�ʼ�����ļ�


typedef void(*pSearchCallFunc)(wchar_t* path, WIN32_FIND_DATAW* pfd,LPVOID param);
typedef void(*pSearchEndCall)(LPVOID Param);

class CSearchFile
{
	struct SEARCH_THREAD_CONTEXT
	{
		unsigned int				ThreadId;									//�߳�ID
		HANDLE						hThread;									//��¼�߳̾��
		CSearchFile*	pThis;										//�������ָ��
	};
	
	struct DEST_FILE_INFO
	{
		WCHAR FileName[4096];
		WCHAR StartDir[4096];
		DWORD FileAttrib;
	};
private:
	//-----------------------------------ջ---------------------------
	LPWSTR*						m_DirStack;
	LPWSTR*						m_StackTop;
	DWORD						m_StackSize;
	void Push(WCHAR*pDir);
	LPWSTR Pop();
	//----------------------------------�߳�--------------------------
	DWORD						m_dwThreadCount;								//��¼�����߳�����
	volatile long				m_lWorkingThreadCount;							//���ڹ����Ĺ������߳�����
	DWORD						m_dwMasterThreadId;								//���߳�ID
	HANDLE						m_hMasterThread;
	//----------------------------------״̬-------------------------
	volatile long				m_bStopSearching ;								//�Ƿ�ֹͣ����
	//----------------------------------����-------------------------
	
	HANDLE						m_hInit;													
	HANDLE						m_hSearchEnd;									//���ҽ���

	//CRITICAL_SECTION			m_csStack;										//ջ�����ٽ���
	volatile long				m_StackLock;									//������

	DEST_FILE_INFO				m_DestFile;										//Ŀ���ļ���Ϣ
	
	pSearchCallFunc				m_pCallbakcAfterFind;							//���ҵ�һ���ļ��󽫻���ô˺���
	pSearchEndCall				m_pCallbakcAfterOver;							//����֮������������
	LPVOID						m_Param;										//�������
	
	static void MasterThread(CSearchFile*pThis);					//���߳�
	static unsigned int __stdcall WorkerThread(void*);		//�����߳�
public:
	int Search(LPWSTR DestFileName, LPWSTR StartDir, DWORD FileAttribs,
		pSearchCallFunc pfun1,pSearchEndCall pfun2, LPVOID CallbackParam);		//�����ļ�

	void StopSearching();														//ֹͣ����
	void WaitForTheEndOfSearch();												//�ȴ����ҽ���
	BOOL IsSearching();															//�Ƿ����ڲ���
	CSearchFile(DWORD ThreadCount = 0);
	~CSearchFile();
};

