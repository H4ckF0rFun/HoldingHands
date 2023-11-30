#pragma once
#include "EventHandler.h"


#define PROCESS_MANAGER ('P'|('R'<<8)|('O'<<16)|('C'<<24))

#define PROCESS_MANAGER_ERROR	(0x5500)

#define PROCESS_MANAGER_MODIFY	(0x5501)
#define PROCESS_MANAGER_APPEND	(0x5502)
#define PROCESS_MANAGER_REMOVE	(0x5503)

#define PROCESS_MANAGER_PROCESS_COUNT_MODIFY	(0x5504)
#define PROCESS_MANAGER_CPU_USAGE_MODIFY		(0x5506)
#define PROCESS_MANAGER_MEMORY_USAGE_MODIFY		(0x5507)

#define PROCESS_MANAGER_GROW_IMAGE_LIST			(0x5508)
#define PROCESS_KILL_PROCESS					(0x5509)


//notify message


#define WM_PROCESS_MANAGER_MODIFY	(WM_USER + 0x100)
#define WM_PROCESS_MANAGER_APPEND	(WM_USER + 0x101)
#define WM_PROCESS_MANAGER_REMOVE	(WM_USER + 0x102)

#define WM_PROCESS_MANAGER_PROCESS_COUNT_MODIFY		(WM_USER + 0x103)
#define WM_PROCESS_MANAGER_CPU_USAGE_MODIFY			(WM_USER + 0x104)
#define WM_PROCESS_MANAGER_MEMORY_USAGE_MODIFY		(WM_USER + 0x105)


//
#define WM_PROCESS_MANAGER_APPEND_ICON				(WM_USER + 0x106)

#define MODIFY_UPDATE_NAME				0x1
#define MODIFY_UPDATE_PATH				0x2
#define MODIFY_UPDATE_USER				0x4

#define MODIFY_UPDATE_PID				0x800
#define MODIFY_UPDATE_PPID				0x8
#define MODIFY_UPDATE_PRIO				0x10
#define MODIFY_UPDATE_MODS				0x20
#define MODIFY_UPDATE_HADS				0x40
#define MODIFY_UPDATE_THDS				0x80
#define MODIFY_UPDATE_MEM				0x100
#define MODIFY_UPDATE_CPU				0x200
#define MODIFY_UPDATE_ICON				0x400


typedef struct tagProcessInfo
{
	TCHAR szName[0x100];
	TCHAR szExePath[0x100];
	TCHAR szUser[0x100];

	DWORD dwPid;						//进程ID
	DWORD dwParentPid;					//父进程ID
	DWORD dwPriority;					//优先级.

	DWORD dwModules;					//模块数量
	DWORD dwHandles;					//句柄数量
	DWORD dwThreads;					//线程数量.
	DWORD dwWorkSet;					//活动的占用内存.
	DWORD dwCPUUsage;


	DWORD dwIconIndex;


	LARGE_INTEGER liLastSystemTime;		//上一时刻占用的系统时间
	LARGE_INTEGER liLastTime;			//上一次开始计时的时间.
}ProcessInfo;


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

