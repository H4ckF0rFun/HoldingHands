#include <Windows.h>
/*
��DLL���ڰ�װȫ�ּ��̹���,Զ������ʱ�������а���.
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

extern "C" __declspec(dllexport) HWND	hTopWindow = NULL;		//HoldingHands RemoteDesktop ��ǰ���㴰��
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
			//������Ϣ����RemoteDesktop ����
			KBDLLHOOKSTRUCT*pKbInfo = (KBDLLHOOKSTRUCT*)lParam;
			PostMessage(hTopWindow, wParam, pKbInfo->vkCode, 0);
			return 1;			//���뷵�ط�0ֵ.
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}