#pragma once
#include "EventHandler.h"
#include "process_manager_common.h"
#include <map>
#include <initguid.h>
#include <CommCtrl.h>
#include <commoncontrols.h>
#include "module.h"

class CProcessManager :
	public CEventHandler
{


private:
	HANDLE m_hEvent;
	HANDLE m_hThread;

	DWORD m_dwProcessCount;
	DWORD m_dwCpuUsage;
	DWORD m_dwMemUsage; 

	DWORD	m_ImageListSize;

	std::map <DWORD, ProcessInfo> m_ProcessList;

	DWORD ProcessInfoChanged(const ProcessInfo & left, const ProcessInfo & right);

	static HIMAGELIST hImageList_SmallIcon;
	
	Module  * m_owner;
	
public:

	static spinlock_t m_mutex;

	int   GetExeIconIndex(const TCHAR* ExeFileName);
	DWORD GetIconDataByIndex(int dwIconIndex, BYTE ** lppData);

	CProcessManager(CClient * pClient, Module * owner);
	void OnOpen();
	void OnClose();

	void OnEvent(UINT32 e, BYTE*lpData, UINT Size);
	BOOL UpdateProcessList();

	void Kill(DWORD* Pids,UINT Count);

	static void ThreadProc(CProcessManager * pManager);
	virtual ~CProcessManager();
};

