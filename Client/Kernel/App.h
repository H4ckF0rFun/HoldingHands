#pragma once
#include "Kernel.h"

class CApp
{
private:
	static DWORD  ServerPort;
	static char   ServerAddress[0x20];
	static SERVICE_STATUS ServiceStatus;
	static SERVICE_STATUS_HANDLE hStatus;
	
	static void Restart();

	static LONG WINAPI CApp::TOP_LEVEL_EXCEPTION_FILTER(
		_In_ struct _EXCEPTION_POINTERS *ExceptionInfo
		);
	static BOOL InstallService();
	static void RunAsService();
	static DWORD WINAPI StartKernel(LPVOID Param);
	static void  WINAPI ServiceHandler(DWORD fdwControl);

	void Run();
public:
	static void  WINAPI  CApp::ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);

	CApp();
	~CApp();
};

