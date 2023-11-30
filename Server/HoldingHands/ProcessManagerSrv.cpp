#include "stdafx.h"
#include "ProcessManagerSrv.h"


CProcessManagerSrv::CProcessManagerSrv(CClient*pClient):
CEventHandler(pClient, PROCESS_MANAGER)
{

}


CProcessManagerSrv::~CProcessManagerSrv()
{

}

void CProcessManagerSrv::OnOpen()
{

}
void CProcessManagerSrv::OnClose()
{
	
}

void CProcessManagerSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case PROCESS_MANAGER_APPEND:
		OnAppendProcess((ProcessInfo*)lpData);
		break;
	case PROCESS_MANAGER_REMOVE:
		OnRemoveProcess(*(DWORD*)lpData);
		break;
	case PROCESS_MANAGER_MODIFY:
		OnModifyProcess((ProcessInfo*)lpData);
		break;

	case PROCESS_MANAGER_CPU_USAGE_MODIFY:
		Notify(WM_PROCESS_MANAGER_CPU_USAGE_MODIFY, *(LPDWORD)lpData);
		break;
	case PROCESS_MANAGER_MEMORY_USAGE_MODIFY:
		Notify(WM_PROCESS_MANAGER_MEMORY_USAGE_MODIFY, *(LPDWORD)lpData);
		break;
	case PROCESS_MANAGER_PROCESS_COUNT_MODIFY:
		Notify(WM_PROCESS_MANAGER_PROCESS_COUNT_MODIFY, *(LPDWORD)lpData);
		break;
	case PROCESS_MANAGER_GROW_IMAGE_LIST:
		OnAppendIcon(lpData,Size);
		break;
	}
}

void CProcessManagerSrv::OnAppendProcess(ProcessInfo * pi)
{
	Notify(WM_PROCESS_MANAGER_APPEND, (WPARAM)pi);
}

void CProcessManagerSrv::OnModifyProcess(ProcessInfo* pi)
{
	DWORD dwFlags = *(DWORD*)(pi + 1);
	Notify(WM_PROCESS_MANAGER_MODIFY, (WPARAM)pi, dwFlags);
}

void CProcessManagerSrv::OnRemoveProcess(DWORD dwPid)
{
	Notify(WM_PROCESS_MANAGER_REMOVE, dwPid);
}

void CProcessManagerSrv::OnAppendIcon(BYTE * lpIconData,DWORD dwSize)
{
	HICON hIcon				= 0;
	ICONINFO * lpIconInfo   = (ICONINFO*)lpIconData;
	
	BITMAP	 * lpMaskBitmap = (BITMAP*)(lpIconInfo + 1);
	BYTE	 * lpMaskBits   = (BYTE*)(lpMaskBitmap + 1);

	BITMAP	 * lpColorBitmap = NULL;
	BYTE	 * lpColorBits   = NULL;
	//
	if (dwSize == 0)
	{
		Close();
		return;
	}

	if (lpIconInfo->hbmColor)
	{
		lpColorBitmap = (BITMAP*)(lpMaskBits + lpMaskBitmap->bmHeight * lpMaskBitmap->bmWidthBytes);
		lpColorBits   = (BYTE*)(lpColorBitmap + 1);
	}
	
	lpMaskBitmap->bmBits = lpMaskBits;
	if (lpColorBitmap)
		lpColorBitmap->bmBits = lpColorBits;

	lpIconInfo->hbmMask = CreateBitmapIndirect(lpMaskBitmap);

	if (lpColorBitmap)
		lpIconInfo->hbmColor = CreateBitmapIndirect(lpColorBitmap);

	//
	hIcon = CreateIconIndirect(lpIconInfo); 

	if (hIcon)
		Notify(WM_PROCESS_MANAGER_APPEND_ICON, (WPARAM)hIcon);
	else
		Close();

	//clean...
	if (lpIconInfo->hbmMask)
		DeleteObject(lpIconInfo->hbmMask);

	if (lpIconInfo->hbmColor)
		DeleteObject(lpIconInfo->hbmColor);

	if (hIcon)
		DestroyIcon(hIcon);
}

void CProcessManagerSrv::KillProcess(DWORD Pid)
{
	Send(PROCESS_KILL_PROCESS, &Pid, sizeof(Pid));
}