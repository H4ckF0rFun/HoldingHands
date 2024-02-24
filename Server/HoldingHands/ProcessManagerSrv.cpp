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
	Notify(WM_PROCESS_MANAGER_ERROR,(WPARAM)TEXT("Connection close"),0);
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
	HICON    hIcon		 = 0;
	ICONINFO icon_info   = { 0 };
	BITMAP	 MaskBitmap  = { 0 };
	BITMAP   ColorBitmap = { 0 };


	BYTE	 * lpMaskBits    = NULL;
	BYTE	 * lpColorBits   = NULL;
	//

	PROCESS_ICONINFO * processIconInfo = (PROCESS_ICONINFO*)lpIconData;
	PROCESS_BITMAP   * MaskBmp = (PROCESS_BITMAP*)(processIconInfo + 1); 
	PROCESS_BITMAP	 * ColorBmp = NULL;

	lpMaskBits = (LPBYTE)(MaskBmp + 1);

	if (dwSize == 0)
	{
		Close();
		return;
	}

	if (processIconInfo->bHasBMColor)
	{
		ColorBmp	= (PROCESS_BITMAP*)(lpMaskBits + MaskBmp->bmHeight * MaskBmp->bmWidthBytes);
		lpColorBits = (BYTE*)(ColorBmp + 1);
	}
	
	icon_info.fIcon = processIconInfo->fIcon;
	icon_info.xHotspot = processIconInfo->xHotspot;
	icon_info.yHotspot = processIconInfo->yHotspot;

	MaskBitmap.bmType = MaskBmp->bmType;
	MaskBitmap.bmHeight = MaskBmp->bmHeight;
	MaskBitmap.bmWidth = MaskBmp->bmWidth;
	MaskBitmap.bmWidthBytes = MaskBmp->bmWidthBytes;
	MaskBitmap.bmPlanes = MaskBmp->bmPlanes;
	MaskBitmap.bmBitsPixel = MaskBmp->bmBitsPixel;
	MaskBitmap.bmBits = lpMaskBits;

	if (lpColorBits)
	{
		ColorBitmap.bmType = ColorBmp->bmType;
		ColorBitmap.bmHeight = ColorBmp->bmHeight;
		ColorBitmap.bmWidth = ColorBmp->bmWidth;
		ColorBitmap.bmWidthBytes = ColorBmp->bmWidthBytes;
		ColorBitmap.bmPlanes = ColorBmp->bmPlanes;
		ColorBitmap.bmBitsPixel = ColorBmp->bmBitsPixel;
		ColorBitmap.bmBits = lpColorBits;

	}

	icon_info.hbmMask = CreateBitmapIndirect(&MaskBitmap);

	if (ColorBmp)
		icon_info.hbmColor = CreateBitmapIndirect(&ColorBitmap);

	//
	hIcon = CreateIconIndirect(&icon_info);

	if (hIcon)
		Notify(WM_PROCESS_MANAGER_APPEND_ICON, (WPARAM)hIcon);
	else
		Close();

	//clean...
	if (icon_info.hbmMask)
		DeleteObject(icon_info.hbmMask);

	if (icon_info.hbmColor)
		DeleteObject(icon_info.hbmColor);

	if (hIcon)
		DestroyIcon(hIcon);
}

void CProcessManagerSrv::KillProcess(DWORD Pid)
{
	Send(PROCESS_KILL_PROCESS, &Pid, sizeof(Pid));
}