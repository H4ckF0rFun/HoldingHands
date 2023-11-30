#include <Windows.h>
/*
该DLL用于安装全局键盘钩子,远程桌面时屏蔽所有按键.
*/

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to the DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved   // reserved
	)
{
	return TRUE;
}

#pragma data_seg("Shared")     

extern "C" __declspec(dllexport) HWND	hTopWindow = NULL;		//HoldingHands RemoteDesktop 当前顶层窗口
#pragma data_seg()
extern "C" __declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK LowLevelKeyboardProc(
	int nCode,     // hook code
	WPARAM wParam, // message identifier
	LPARAM lParam  // message data
	)
{
	if (nCode == HC_ACTION)
	{
		HWND hForegroundWnd = GetForegroundWindow();
		if (hTopWindow == hForegroundWnd)
		{
			//按键消息发给RemoteDesktop 窗口
			KBDLLHOOKSTRUCT*pKbInfo = (KBDLLHOOKSTRUCT*)lParam;
			PostMessage(hTopWindow, wParam, pKbInfo->vkCode, 0);
			return 1;			//必须返回非0值.
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}