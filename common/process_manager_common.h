#ifndef PROCESS_MANAGER_COMMON_H
#define PROCESS_MANAGER_COMMON_H


#define PROCESS_MANAGER ('P'|('R'<<8)|('O'<<16)|('C'<<24))

enum PROCESS_MANAGER_EVENT
{
	PROCESS_MANAGER_ERROR,
	PROCESS_MANAGER_MODIFY,
	PROCESS_MANAGER_APPEND,
	PROCESS_MANAGER_REMOVE,
	PROCESS_MANAGER_PROCESS_COUNT_MODIFY,
	PROCESS_MANAGER_CPU_USAGE_MODIFY,
	PROCESS_MANAGER_MEMORY_USAGE_MODIFY,
	PROCESS_MANAGER_GROW_IMAGE_LIST,
	PROCESS_KILL_PROCESS
};

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
	DWORD dwCPUUsage;					//

	DWORD dwIconIndex;

	LARGE_INTEGER liLastSystemTime;		//上一时刻占用的系统时间
	LARGE_INTEGER liLastTime;			//上一次开始计时的时间.
}ProcessInfo;



#define MODIFY_UPDATE_NAME				0x1
#define MODIFY_UPDATE_PATH				0x2
#define MODIFY_UPDATE_USER				0x4
#define MODIFY_UPDATE_PPID				0x8
#define MODIFY_UPDATE_PRIO				0x10
#define MODIFY_UPDATE_MODS				0x20
#define MODIFY_UPDATE_HADS				0x40
#define MODIFY_UPDATE_THDS				0x80
#define MODIFY_UPDATE_MEM				0x100
#define MODIFY_UPDATE_CPU				0x200
#define MODIFY_UPDATE_ICON				0x400
#define MODIFY_UPDATE_PID				0x800

#endif