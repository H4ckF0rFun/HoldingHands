#include "App.h"
#include <Shlwapi.h>

#define SERVICE_NAME		   TEXT("TestService")

DWORD                 CApp::ServerPort          = 10086;
char                  CApp::ServerAddress[0x20] = "127.0.0.1";
SERVICE_STATUS        CApp::ServiceStatus;
SERVICE_STATUS_HANDLE CApp::hStatus;

CApp::CApp()
{
	Run();
}


CApp::~CApp()
{

}

BOOL CApp::InstallService()
{
	SC_HANDLE hServiceMgr = NULL;
	SC_HANDLE hService = NULL;
	BOOL	  bResult = TRUE;
	TCHAR     FilePath[MAX_PATH];

	hServiceMgr = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		return FALSE;
	}

	GetModuleFileName(NULL, FilePath, MAX_PATH);
	lstrcat(FilePath, TEXT(" -AsService"));

	hService = CreateService(
		hServiceMgr,
		SERVICE_NAME,					// service name
		SERVICE_NAME,					// display name
		SERVICE_ALL_ACCESS,				// 
		SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,		// service type
		SERVICE_AUTO_START,				// start type
		SERVICE_ERROR_IGNORE,			// error control
		FilePath,						// binary path
		NULL,							// group
		NULL,
		NULL,						    // dependency
		NULL,
		NULL);

	if (hService == NULL && GetLastError() == ERROR_SERVICE_EXISTS)
	{
		hService = OpenService(hServiceMgr, SERVICE_NAME, SERVICE_START);
	}

	if (hService == NULL)
		bResult = FALSE;
	
	if (hService && !StartServiceA(hService, 0, NULL))
	{
		if (ERROR_SERVICE_ALREADY_RUNNING != GetLastError())
			bResult = FALSE;
	}

	if (hService)
		CloseServiceHandle(hService);

	if (hServiceMgr)
		CloseServiceHandle(hServiceMgr);    // SCM句柄

	return bResult;
}

void WINAPI CApp::ServiceHandler(DWORD fdwControl)
{
	switch (fdwControl)
	{
	case SERVICE_CONTROL_PAUSE:
		ServiceStatus.dwCurrentState = SERVICE_PAUSED;
		break;
	case SERVICE_CONTROL_CONTINUE:
		ServiceStatus.dwCurrentState = SERVICE_RUNNING;
		break;
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwCheckPoint = 0;
		ServiceStatus.dwWaitHint = 0;
		break;
	}
	SetServiceStatus(hStatus, &ServiceStatus);
	return;
}

LONG WINAPI CApp::TOP_LEVEL_EXCEPTION_FILTER(
	_In_ struct _EXCEPTION_POINTERS *ExceptionInfo
	){
	
	//SaveCrashInfo(ExceptionInfo);

	ExceptionInfo->ContextRecord->Eip = (DWORD)Restart;
	return EXCEPTION_CONTINUE_EXECUTION;
}


void CApp::Restart()
{
	TCHAR				szFileName[MAX_PATH];
	STARTUPINFO			si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };

	GetModuleFileName(GetModuleHandle(0), szFileName, MAX_PATH);

	if (FALSE == CreateProcess(szFileName, 0, 0, 0, 0, 0, 0, 0, &si, &pi))
	{
		StartKernel(NULL);
		return;
	}
	ExitProcess(0);
}


void WINAPI CApp::ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	HANDLE hThread = NULL;

	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = 
		SERVICE_ACCEPT_STOP | 
		SERVICE_ACCEPT_PAUSE_CONTINUE | 
		SERVICE_ACCEPT_SHUTDOWN;
	
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwWaitHint = 0;

	hStatus = RegisterServiceCtrlHandler(
		SERVICE_NAME,
		(LPHANDLER_FUNCTION)ServiceHandler);

	if (!hStatus)
	{
		return;
	}

	SetServiceStatus(hStatus, &ServiceStatus);

	if (GetLastError() != NO_ERROR)
	{
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwCheckPoint = 0;
		ServiceStatus.dwWaitHint = 0;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
	}

	// 从这里开始可以放入你想服务为你所做的事情。  
	hThread = CreateThread(NULL, 0, StartKernel, NULL, 0, NULL);
	CloseHandle(hThread);

	//set Service statu.
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;
	SetServiceStatus(hStatus, &ServiceStatus);
}

void CApp::RunAsService()
{
	SERVICE_TABLE_ENTRY ServTable[2];
	ServTable[0].lpServiceName = SERVICE_NAME;
	ServTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

	ServTable[1].lpServiceName = NULL;
	ServTable[1].lpServiceProc = NULL;

	//Start Service 会阻塞,然后Stop服务的时候才会返回.
	StartServiceCtrlDispatcher(ServTable);
}


DWORD WINAPI CApp::StartKernel(LPVOID Param)
{
	/*CIOCPClient::SocketInit();
	CIOCPClient client(
			ServerAddress, 
			ServerPort,
			TRUE,
			INFINITE);

	CKernel  kernel(&client);
	client.Run();
	CIOCPClient::SocketTerm();*/
	return 0;
}

void CApp::Run()
{
	char * szCmdLine = GetCommandLineA();
	
	if (StrStrA(szCmdLine, " -AsService"))
	{
		RunAsService();
	}
	else if (!InstallService())
	{
		StartKernel(NULL);
	}
}