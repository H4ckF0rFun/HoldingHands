#pragma once
#include "EventHandler.h"
#include "process_manager_common.h"


//notify message


#define WM_PROCESS_MANAGER_MODIFY	(WM_USER + 0x100)
#define WM_PROCESS_MANAGER_APPEND	(WM_USER + 0x101)
#define WM_PROCESS_MANAGER_REMOVE	(WM_USER + 0x102)

#define WM_PROCESS_MANAGER_PROCESS_COUNT_MODIFY		(WM_USER + 0x103)
#define WM_PROCESS_MANAGER_CPU_USAGE_MODIFY			(WM_USER + 0x104)
#define WM_PROCESS_MANAGER_MEMORY_USAGE_MODIFY		(WM_USER + 0x105)


//
#define WM_PROCESS_MANAGER_APPEND_ICON				(WM_USER + 0x106)
#define WM_PROCESS_MANAGER_ERROR			        (WM_USER + 0x107)

class CProcessManagerSrv :
	public CEventHandler
{
public:
	void OnOpen();
	void OnClose();
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);

	void OnAppendProcess(ProcessInfo * pi);
	void OnRemoveProcess(DWORD dwPid);
	void OnModifyProcess(ProcessInfo* pi);
	void OnAppendIcon(BYTE * lpIconData, DWORD dwSize);

	void KillProcess(DWORD Pid);
	CProcessManagerSrv(CClient*pClient);
	virtual ~CProcessManagerSrv();
};

