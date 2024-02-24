#include "ProcessManager.h"
#include <TlHelp32.h>
#include <shellapi.h>
#include "utils.h"
#include <psapi.h>
#include <stdint.h>
#include <assert.h>

#pragma comment(lib,"Comctl32")

HIMAGELIST CProcessManager::hImageList_SmallIcon = NULL;



spinlock_t CProcessManager::m_mutex = 0;

CProcessManager::CProcessManager(CClient * pClient, Module * owner) :
CEventHandler(pClient, PROCESS_MANAGER)
{
	m_hEvent = NULL;
	m_hThread = NULL;

	m_dwMemUsage = 0;
	m_dwCpuUsage = 0;
	m_dwProcessCount = 0;
	
	m_ImageListSize = 0;				//已经发送的ImageList 大小.

	__spin_lock(CProcessManager::m_mutex);

	if (hImageList_SmallIcon == NULL)
	{
		SHGetImageList(SHIL_SMALL, IID_IImageList, (void**)&hImageList_SmallIcon);
	}

	__spin_unlock(CProcessManager::m_mutex);

	m_owner = owner;
	if (m_owner)
		get_module(m_owner);
}


CProcessManager::~CProcessManager()
{
	CloseHandle(m_hEvent);
	m_hEvent = NULL;

	if (m_owner)
		put_module(m_owner);
}



/*
name pid parent_pid user CPU memory handles threads commandline description DEP
ok   ok  ok                                 ok
*/


BOOL DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath)
{
	TCHAR			szDriveStr[500];
	TCHAR			szDrive[3];
	TCHAR			szDevName[100];
	INT				cchDevName;
	INT				i;

	if (!pszDosPath || !pszNtPath)
		return FALSE;

	if (GetLogicalDriveStrings(sizeof(szDriveStr), szDriveStr))
	{
		for (i = 0; szDriveStr[i]; i += 4)
		{
			if (!lstrcmpi(&(szDriveStr[i]), TEXT("A:\\")) || !lstrcmpi(&(szDriveStr[i]), TEXT("B:\\")))
				continue;

			szDrive[0] = szDriveStr[i];
			szDrive[1] = szDriveStr[i + 1];
			szDrive[2] = '\0';

			if (!QueryDosDevice(szDrive, szDevName, 100))//查询 Dos 设备名
				return FALSE;

			cchDevName = lstrlen(szDevName);

			if (!memcmp(pszDosPath, szDevName, sizeof(TCHAR) * cchDevName))//命中
			{
				lstrcpy(pszNtPath, szDrive);//复制驱动器
				lstrcat(pszNtPath, pszDosPath + cchDevName);//复制路径
				return TRUE;
			}
		}
	}
	lstrcpy(pszNtPath, pszDosPath);
	return FALSE;

}


BOOL GetProcessUserName(HANDLE hProcess, TCHAR * szUser)
{
	HANDLE hToken = NULL;
	BOOL   bRet = FALSE;
	PTOKEN_USER pToken_User = NULL;
	DWORD dwTokenUser = 0;
	TCHAR szAccName[MAX_PATH] = { 0 };
	TCHAR szDomainName[MAX_PATH] = { 0 };
	HANDLE hProcessToken = NULL;

	szUser[0] = 0;

	if (hProcess == NULL)
		return FALSE;

	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) // 失败
		return FALSE;

	if (hToken == NULL)
		return FALSE;


	GetTokenInformation(hToken, TokenUser, NULL, 0L, &dwTokenUser);

	if (dwTokenUser>0)
	{
		pToken_User = (PTOKEN_USER)::GlobalAlloc(GPTR, dwTokenUser);
	}

	if (pToken_User != NULL)
	{
		bRet = GetTokenInformation(hToken, TokenUser, pToken_User, dwTokenUser, &dwTokenUser);
	}

	if (bRet && pToken_User)
	{
		SID_NAME_USE eUse = SidTypeUnknown;
		DWORD dwAccName = 0L;
		DWORD dwDomainName = 0L;
		PSID  pSid = pToken_User->User.Sid;
		bRet = LookupAccountSidA(
			NULL,
			pSid,
			NULL,
			&dwAccName,
			NULL,
			&dwDomainName,
			&eUse
			);

		if (dwAccName>0 && dwAccName < MAX_PATH && dwDomainName>0 && dwDomainName <= MAX_PATH)
		{
			bRet = LookupAccountSid(
				NULL,
				pSid,
				szAccName,
				&dwAccName,
				szDomainName,
				&dwDomainName,
				&eUse
				);
		}

		if (bRet)
			lstrcpy(szUser, szAccName);
	}

	if (pToken_User != NULL)
		GlobalFree(pToken_User);

	if (hToken != NULL)
		CloseHandle(hToken);

	return TRUE;
}

int CProcessManager::GetExeIconIndex(const TCHAR* ExeFileName)
{
	SHFILEINFO  si = { 0 };
	DWORD		dwNewImageListSize = 0;
	BYTE*		lpIconData = 0;
	DWORD		dwIconDataSize = 0;

	//添加到list里面
	SHGetFileInfo(
		ExeFileName,
		FILE_ATTRIBUTE_NORMAL,
		&si,
		sizeof(si),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX
		);


	dwNewImageListSize = ImageList_GetImageCount(hImageList_SmallIcon);

	if (dwNewImageListSize > m_ImageListSize)
	{
		//UpdateImageList.
		for (int i = m_ImageListSize; i < dwNewImageListSize; i++)
		{
			dwIconDataSize = GetIconDataByIndex(i,&lpIconData);
			assert(dwIconDataSize); 
			Send(PROCESS_MANAGER_GROW_IMAGE_LIST, lpIconData, dwIconDataSize);
		}
		m_ImageListSize = ImageList_GetImageCount(hImageList_SmallIcon);
	}
	return si.iIcon;
}



DWORD CProcessManager::GetIconDataByIndex(int dwIconIndex, BYTE ** lppData)
{
	static BYTE IconData[0x2000];			//0x200的足够了..
	HICON       hIcon      = NULL;
	ICONINFO	icon_info  = { 0 };
	BITMAP		bmp;
	BYTE *		lpIconData = IconData;


	hIcon = ImageList_GetIcon(hImageList_SmallIcon, dwIconIndex, 0);

	if (!hIcon)
		goto failed;

	GetIconInfo(hIcon, &icon_info);

	//Write Icon Info
	((PROCESS_ICONINFO*)lpIconData)->fIcon = icon_info.fIcon;
	((PROCESS_ICONINFO*)lpIconData)->xHotspot = icon_info.xHotspot;
	((PROCESS_ICONINFO*)lpIconData)->yHotspot = icon_info.yHotspot;
	((PROCESS_ICONINFO*)lpIconData)->bHasBMColor = icon_info.hbmColor != NULL;
	((PROCESS_ICONINFO*)lpIconData)->bHasBMMask = icon_info.hbmMask != NULL;
	
	lpIconData += sizeof(PROCESS_ICONINFO);

	//先写Mask Data
	GetObject(icon_info.hbmMask, sizeof(BITMAP), &bmp);
	((PROCESS_BITMAP*)lpIconData)->bmType = bmp.bmType;
	((PROCESS_BITMAP*)lpIconData)->bmWidth = bmp.bmWidth;
	((PROCESS_BITMAP*)lpIconData)->bmHeight = bmp.bmHeight;
	((PROCESS_BITMAP*)lpIconData)->bmWidthBytes = bmp.bmWidthBytes;
	((PROCESS_BITMAP*)lpIconData)->bmPlanes = bmp.bmPlanes;
	((PROCESS_BITMAP*)lpIconData)->bmBitsPixel = bmp.bmBitsPixel;
	lpIconData += sizeof(PROCESS_BITMAP);

	//写bits
	GetBitmapBits(icon_info.hbmMask, bmp.bmWidthBytes * bmp.bmHeight, lpIconData);
	lpIconData += bmp.bmWidthBytes * bmp.bmHeight;

	//if exist hbmColor
	if (icon_info.hbmColor)
	{
		//写 Color bitmap.
		GetObject(icon_info.hbmColor, sizeof(BITMAP), &bmp);
		((PROCESS_BITMAP*)lpIconData)->bmType = bmp.bmType;
		((PROCESS_BITMAP*)lpIconData)->bmWidth = bmp.bmWidth;
		((PROCESS_BITMAP*)lpIconData)->bmHeight = bmp.bmHeight;
		((PROCESS_BITMAP*)lpIconData)->bmWidthBytes = bmp.bmWidthBytes;
		((PROCESS_BITMAP*)lpIconData)->bmPlanes = bmp.bmPlanes;
		((PROCESS_BITMAP*)lpIconData)->bmBitsPixel = bmp.bmBitsPixel;
		lpIconData += sizeof(PROCESS_BITMAP);

		//写bits
		GetBitmapBits(icon_info.hbmColor, bmp.bmWidthBytes * bmp.bmHeight, lpIconData);
		lpIconData += bmp.bmWidthBytes * bmp.bmHeight;
	}

failed:
	if (icon_info.hbmMask)
	{
		DeleteObject(icon_info.hbmMask);
	}
	if (icon_info.hbmColor)
	{
		DeleteObject(icon_info.hbmColor);
	}
	if (hIcon)
	{
		DestroyIcon(hIcon);
	}

	*lppData = IconData;
	return (lpIconData - IconData);
}


DWORD CProcessManager::ProcessInfoChanged(const ProcessInfo & left, const ProcessInfo & right)
{
	DWORD ChangedFlag = 0;

	if (lstrcmp(left.szName, right.szName))
		ChangedFlag |= MODIFY_UPDATE_NAME;
	
	if (lstrcmp(left.szExePath, right.szExePath))
		ChangedFlag |= MODIFY_UPDATE_PATH;
	
	if (lstrcmp(left.szUser, right.szUser))
		ChangedFlag |= MODIFY_UPDATE_USER;
	//
	if (left.dwPid != right.dwPid)
		ChangedFlag |= MODIFY_UPDATE_PID;

	if (left.dwParentPid != right.dwParentPid)
		ChangedFlag |= MODIFY_UPDATE_PPID;
	
	if (left.dwPriority != right.dwPriority)
		ChangedFlag |= MODIFY_UPDATE_PRIO;
	
	if (left.dwModules != right.dwModules)
		ChangedFlag |= MODIFY_UPDATE_MODS;
	
	if (left.dwHandles != right.dwHandles)
		ChangedFlag |= MODIFY_UPDATE_HADS;
	
	if (left.dwThreads != right.dwThreads)
		ChangedFlag |= MODIFY_UPDATE_THDS;
	
	if (left.dwWorkSet != right.dwWorkSet)
		ChangedFlag |= MODIFY_UPDATE_MEM;
	
	if (left.dwIconIndex != right.dwIconIndex)
		ChangedFlag |= MODIFY_UPDATE_ICON;
	
	if (left.dwCPUUsage != right.dwCPUUsage)
		ChangedFlag |= MODIFY_UPDATE_CPU;

	return ChangedFlag;
}



/// 时间转换  
static uint64_t FileTime2UTC(const FILETIME* ftime)
{
	LARGE_INTEGER li;
	li.LowPart = ftime->dwLowDateTime;
	li.HighPart = ftime->dwHighDateTime;
	return li.QuadPart;
}


float GetCpuUsageByProcess(ProcessInfo * pi,HANDLE hProcess)
{
	//cpu数量  
	static DWORD cntProcess = 0;
	FILETIME     now;
	FILETIME	 creation_time;
	FILETIME	 exit_time;
	FILETIME	 kernel_time;
	FILETIME	 user_time;
	int64_t		 system_time;
	int64_t		 time;
	int64_t		 system_time_delta;
	int64_t		 time_delta;
	float		 CPU_Usage = 0;

	if (!cntProcess)
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		cntProcess = info.dwNumberOfProcessors;
	}

	//CPU占用率 = 程序执行时间 / 单位时间.
	if (!GetProcessTimes(
		hProcess,
		&creation_time,
		&exit_time,
		&kernel_time,
		&user_time)
		)
	{
		return 0;
	}

	
	system_time = (FileTime2UTC(&kernel_time) + FileTime2UTC(&user_time)) / cntProcess;

	GetSystemTimeAsFileTime(&now);
	time = FileTime2UTC(&now);


	//first calc..
	if ((pi->liLastSystemTime.QuadPart == 0) || (pi->liLastTime.QuadPart == 0))
	{
		pi->liLastSystemTime.QuadPart = system_time;
		pi->liLastTime.QuadPart = time;
		return 0;
	}

	system_time_delta = system_time - pi->liLastSystemTime.QuadPart;
	time_delta        = time        - pi->liLastTime.QuadPart;

	if (!time_delta)
		return 0;

	CPU_Usage = ((system_time_delta * 100.0) / time_delta);

	//update system time and time.
	pi->liLastSystemTime.QuadPart = system_time;
	pi->liLastTime.QuadPart       = time;

	return CPU_Usage;
}


BOOL CProcessManager::UpdateProcessList()
{
	BOOL			bFirst = TRUE;
	BOOL			bNext = TRUE;
	HANDLE			hSnapshot = NULL;
	HANDLE			hProcess = NULL;
	HMODULE			hModules = NULL;
	PROCESSENTRY32	pe32 = { 0 };
	DWORD			cbNeeded;
	TCHAR           szImagePath[MAX_PATH] = { 0 };
	float			CpuUsage = 0;
	DWORD			dwCpuUsage = 0;
	MEMORYSTATUS    ms = { 0 };
	char *			lpIconData = 0;
	DWORD			dwIconDataSize = 0;
	double			tempUsage = 0.0;
	ProcessInfo     pi        = { 0 };
	PROCESS_MEMORY_COUNTERS pmc;
	std::map <DWORD, ProcessInfo> TempProcessList;
	//PROCESS_MEMORY_COUNTERS pmc;

	DebugPrivilege(TEXT("SeDebugPrivilege"), TRUE);

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return FALSE;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	bFirst = Process32First(hSnapshot, &pe32);
	
	while (bFirst && bNext)
	{
		hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
			FALSE, 
			pe32.th32ProcessID);

		//Open Process Failed ,skip this process..
		if (hProcess == NULL)
		{
			bNext = Process32Next(hSnapshot, &pe32);
			continue;
		}

		//why skip this process.
		if ((pe32.th32ProcessID != 0) && 
			(pe32.th32ProcessID != 4) && 
			(pe32.th32ProcessID != 8))
		{
			std::map<DWORD, ProcessInfo>::iterator it = m_ProcessList.end();

			ZeroMemory(&pi,sizeof(pi));
			ZeroMemory(&pmc, sizeof(pmc));
			pmc.cb     = sizeof(PROCESS_MEMORY_COUNTERS);

			//Begin get process information...
			//exe 名称
			lstrcpy(pi.szName, pe32.szExeFile);

			//exe 文件路径.
			if (GetProcessImageFileName(hProcess, szImagePath, MAX_PATH))
			{
				DosPathToNtPath(szImagePath, pi.szExePath);
			}

			//用户.
			if (!GetProcessUserName(hProcess, pi.szUser) || !lstrlen(pi.szUser))
			{
				lstrcpy(pi.szUser, TEXT("-"));
			}

			//进程ID
			pi.dwPid = pe32.th32ProcessID;
			//父进程ID.
			pi.dwParentPid = pe32.th32ParentProcessID;
			//优先级
			pi.dwPriority = GetPriorityClass(hProcess);
			//进程模块数量.
			EnumProcessModules(hProcess, &hModules, sizeof(hModules), &cbNeeded);
			pi.dwModules = cbNeeded / sizeof(hModules);
			//句柄数量
			GetProcessHandleCount(hProcess, &pi.dwHandles);
			//线程数量
			pi.dwThreads = pe32.cntThreads;
			//占用内存.
			if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
			{
				pi.dwWorkSet = (DWORD)(pmc.WorkingSetSize / 1024);		//单位为k
			}
			//Get Icon
			pi.dwIconIndex = GetExeIconIndex(pi.szExePath);
			//CPU Usage
			it = m_ProcessList.find(pi.dwPid);

			if (it == m_ProcessList.end())
			{
				tempUsage     = GetCpuUsageByProcess(&pi, hProcess);
				pi.dwCPUUsage = tempUsage * 1000;
				Send(PROCESS_MANAGER_APPEND, (BYTE*)&pi, sizeof(pi));
			}
			else
			{
				DWORD dwChangedFlag = 0;
				//找到进程了..
				pi.liLastSystemTime = it->second.liLastSystemTime;
				pi.liLastTime       = it->second.liLastTime;

				tempUsage     = GetCpuUsageByProcess(&pi, hProcess);
				pi.dwCPUUsage = tempUsage * 1000;

				dwChangedFlag = ProcessInfoChanged(it->second, pi);

				if (dwChangedFlag)
				{
					vec bufs[2];
					bufs[0].lpData = &pi;
					bufs[0].Size = sizeof(pi);

					bufs[1].lpData = &dwChangedFlag;
					bufs[1].Size = sizeof(dwChangedFlag);

					Send(PROCESS_MANAGER_MODIFY, bufs, 2);
				}
				m_ProcessList.erase(it);
			}

			//calc total cpu usage.
			CpuUsage += tempUsage;
			//insert to temp process list..
			TempProcessList.insert(std::pair<DWORD, ProcessInfo>(pi.dwPid, pi));
		}
		CloseHandle(hProcess);//新修改
		bNext = Process32Next(hSnapshot, &pe32);
	}

	//the left proceess in m_ProcessList is not exist process.
	for (auto & it : m_ProcessList)
		Send(
		PROCESS_MANAGER_REMOVE,
		&it.second.dwPid, 
		sizeof(it.second.dwPid)
	);

	//Update Process list.
	m_ProcessList = std::move(TempProcessList); 
	
	//Update Process Count.
	if (m_ProcessList.size() != m_dwProcessCount)
	{
		DWORD newCount = m_ProcessList.size();
		Send(
			PROCESS_MANAGER_PROCESS_COUNT_MODIFY,
			&newCount,
			sizeof(newCount));
	}
	m_dwProcessCount = m_ProcessList.size();

	//update cpu usage
	dwCpuUsage = CpuUsage;
	if (dwCpuUsage != m_dwCpuUsage)
	{
		Send(
			PROCESS_MANAGER_CPU_USAGE_MODIFY, 
			&dwCpuUsage,
			sizeof(dwCpuUsage));
	}
	m_dwCpuUsage = dwCpuUsage;

	//update memory usage..
	GlobalMemoryStatus(&ms);
	if (m_dwMemUsage != ms.dwMemoryLoad)
	{
		Send(
			PROCESS_MANAGER_MEMORY_USAGE_MODIFY, 
			&ms.dwMemoryLoad, 
			sizeof(ms.dwMemoryLoad)
		);
	}
	m_dwMemUsage = ms.dwMemoryLoad;

	//
	DebugPrivilege(TEXT("SeDebugPrivilege"), FALSE);
	CloseHandle(hSnapshot);
	return TRUE;
}


void CProcessManager::OnOpen()
{
	DWORD dwThreadID = 0;

	if (m_hEvent == NULL)
	{
		m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
	
	ResetEvent(m_hEvent);

	m_hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)ThreadProc,
		(LPVOID)this,
		0,
		&dwThreadID);

	if (m_hThread == NULL)
	{
		char szError[0x200];
		wsprintfA(szError, "Create thread failed with error: %d\n", GetLastError());
		Send(PROCESS_MANAGER_ERROR, szError, lstrlenA(szError) + 1);
		Close();
	}
}

void CProcessManager::OnClose()
{
	if (m_hEvent)
	{
		SetEvent(m_hEvent);
	}

	if (m_hThread)
	{
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
}

void CProcessManager::ThreadProc(CProcessManager * pManager)
{
	for (;;)
	{
		DWORD Result = WaitForSingleObject(pManager->m_hEvent,2000);		//每隔1s刷新一次
		if (Result == WAIT_OBJECT_0)
		{
			break;		//OnClose....
		}
		pManager->UpdateProcessList();
	}
}

void CProcessManager::OnEvent(UINT32 e, BYTE*lpData, UINT Size)
{
	switch (e)
	{
	case PROCESS_KILL_PROCESS:
		Kill((DWORD*)lpData, Size / sizeof(DWORD));
		break;
	default:
		break;
	}
}



void CProcessManager::Kill(DWORD* Pids, UINT Count)
{
	for (int i = 0; i < Count; i++)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, Pids[i]);

		if (hProcess == NULL)
			continue;

		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
}