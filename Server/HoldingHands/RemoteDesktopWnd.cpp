#include "stdafx.h"
#include "RemoteDesktopWnd.h"
#include "RemoteDesktopSrv.h"
#include "MainFrm.h"
#include "utils.h"
#include "resource.h"
#include "MainFrm.h"

#pragma comment(lib,"HHRDKbHook.lib")

DWORD			dwWndCount = 0;			//窗口计数
HHOOK			hKbHook = NULL;

extern"C"__declspec(dllimport)	HWND	hTopWindow;						//顶层窗口
extern"C"__declspec(dllimport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);	//钩子函数

CRemoteDesktopWnd::CRemoteDesktopWnd(CRemoteDesktopSrv*pHandler):
	m_dwCaptureFlags(0),
	m_ControlFlags(0),
	m_pHandler(pHandler),
	m_dwDeskWidth(0),
	m_dwDeskHeight(0),
	m_dwMaxHeight(0),
	m_dwMaxWidth(0),
	m_dwLastTime(0),
	m_dwFps(0),
	m_hDC(0),
	m_hNextViewer(NULL),
	m_DisplayMode(0),
	m_OldDisplayMode(0),
	m_bFullScreenStretchMode(FALSE),
	m_dwMaxpFps(30),
	m_dwQuality(QUALITY_LOW),
	m_LastCursor((HCURSOR)-1)
{
	m_DisplayMode |= DISPLAY_TILE;
	//
	if (!dwWndCount)
	{
		hKbHook = SetWindowsHookEx(
			WH_KEYBOARD_LL, 
			LowLevelKeyboardProc,
			GetModuleHandle(TEXT("HHRDKbHook.dll")), 
			NULL
		);
	}
	dwWndCount++;
	m_pHandler->Get();
}


CRemoteDesktopWnd::~CRemoteDesktopWnd()
{
	//构造函数与析构函数是在UI线程,应该不用担心线程安全问题.
	dwWndCount--;

	m_pHandler->Put();

	if (!dwWndCount){
		UnhookWindowsHookEx(hKbHook);
		hKbHook = NULL;
	}
}

BEGIN_MESSAGE_MAP(CRemoteDesktopWnd, CFrameWnd)
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_COMMAND(ID_DISPLAY_STRETCH, &CRemoteDesktopWnd::OnDisplayStretch)
	ON_COMMAND(ID_DISPLAY_TILE, &CRemoteDesktopWnd::OnDisplayTile)
	ON_COMMAND(ID_CAPTURE_MOUSE, &CRemoteDesktopWnd::OnCaptureMouse)
	ON_COMMAND(ID_CONTROL_KEYBOARD, &CRemoteDesktopWnd::OnControlKeyboard)
	ON_COMMAND(ID_CONTROL_MOUSE, &CRemoteDesktopWnd::OnControlMouse)
	ON_COMMAND(ID_CAPTURE_TRANSPARENTWINDOW, &CRemoteDesktopWnd::OnCaptureTransparentwindow)
	ON_UPDATE_COMMAND_UI(ID_CONTROL_MOUSE, &CRemoteDesktopWnd::OnUpdateControlMouse)
	ON_UPDATE_COMMAND_UI(ID_CONTROL_KEYBOARD, &CRemoteDesktopWnd::OnUpdateControlKeyboard)
	ON_UPDATE_COMMAND_UI(ID_CAPTURE_MOUSE, &CRemoteDesktopWnd::OnUpdateCaptureMouse)
	ON_UPDATE_COMMAND_UI(ID_CAPTURE_TRANSPARENTWINDOW, &CRemoteDesktopWnd::OnUpdateCaptureTransparentwindow)
	ON_MESSAGE(WM_REMOTE_DESKTOP_ERROR,OnError)
	ON_MESSAGE(WM_REMOTE_DESKTOP_DRAW, OnDraw)
	ON_MESSAGE(WM_REMOTE_DESKTOP_SIZE,OnDesktopSize)
	ON_MESSAGE(WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT,OnSetClipbdText)
	ON_MESSAGE(WM_REMOTE_DESKTOP_GET_DRAW_WND, OnGetDrawHwnd)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_MAXFPS_10, &CRemoteDesktopWnd::OnMaxfps10)
	ON_COMMAND(ID_MAXFPS_20, &CRemoteDesktopWnd::OnMaxfps20)
	ON_COMMAND(ID_MAXFPS_30, &CRemoteDesktopWnd::OnMaxfps30)
	ON_COMMAND(ID_MAXFPS_NOLIMIT, &CRemoteDesktopWnd::OnMaxfpsNolimit)
	ON_WM_CHANGECBCHAIN()
	ON_WM_DRAWCLIPBOARD()
	ON_COMMAND(ID_DISPLAY_FULLSCREEN, &CRemoteDesktopWnd::OnDisplayFullscreen)
	ON_COMMAND(ID_OTHER_SCREENSHOT, &CRemoteDesktopWnd::OnOtherScreenshot)
	ON_COMMAND(ID_QUALITY_LOW, &CRemoteDesktopWnd::OnQualityLow)
	ON_COMMAND(ID_QUALITY_HIGH, &CRemoteDesktopWnd::OnQualityHigh)
	ON_MESSAGE(WM_REMOTE_DESKTOP_SCREENSHOT, OnScreenShot)
END_MESSAGE_MAP()



void CRemoteDesktopWnd::PostNcDestroy()
{
	delete this;
}

void CRemoteDesktopWnd::OnClose()
{
	//KillTimer(0x10086);

	if (m_hNextViewer)
	{
		ChangeClipboardChain(m_hNextViewer);
		m_hNextViewer = NULL;
	}
	
	if (m_hDC)
	{
		::ReleaseDC(m_hWnd,m_hDC);
		m_hDC = NULL;
	}

	m_pHandler->SetNotifyWindow(NULL);
	m_pHandler->Close();

	DestroyWindow();
}


int CRemoteDesktopWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	char szIP[0x100];
	CString ip;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_Menu.LoadMenu(IDR_RD_MENU);
	SetMenu(&m_Menu);

	//默认是平铺.
	CMenu*pMenu = m_Menu.GetSubMenu(0);
	pMenu->CheckMenuRadioItem(ID_DISPLAY_STRETCH, ID_DISPLAY_TILE, ID_DISPLAY_TILE, MF_BYCOMMAND);
	
	//Set Window Text.
	m_pHandler->GetPeerAddress(szIP);
	ip = szIP;

	m_Title.Format(TEXT("[%s] RemoteDesktop"), ip.GetBuffer());
	SetWindowText(m_Title);

	//
	m_hDC = ::GetDC(GetSafeHwnd());
	SetStretchBltMode(m_hDC,HALFTONE);
	SetBkMode(m_hDC,TRANSPARENT);
	SetTextColor(m_hDC,0x000033ff);

	pMenu->GetSubMenu(2)->CheckMenuRadioItem(ID_MAXFPS_10, ID_MAXFPS_NOLIMIT, ID_MAXFPS_30, MF_BYCOMMAND);
	pMenu->GetSubMenu(3)->CheckMenuRadioItem(ID_QUALITY_LOW, ID_QUALITY_HIGH, ID_QUALITY_LOW, MF_BYCOMMAND);
	//
	//监听剪切板数据
	m_hNextViewer = SetClipboardViewer();
	
	//
	m_pHandler->SetNotifyWindow(GetSafeHwnd());
	m_pHandler->StartRDP(m_dwMaxpFps, m_dwQuality);
	return 0;
}

LRESULT CRemoteDesktopWnd::OnGetDrawHwnd(WPARAM wParam, LPARAM lParam)
{
	return (LRESULT)GetSafeHwnd();
}

void CRemoteDesktopWnd::OnDisplayStretch()
{
	// TODO:  在此添加命令处理程序代码

	m_DisplayMode = DISPLAY_STRETCH;
	CMenu*pMenu = m_Menu.GetSubMenu(0);
	EnableScrollBarCtrl(SB_VERT, 0);
	EnableScrollBarCtrl(SB_HORZ, 0);
	pMenu->CheckMenuRadioItem(ID_DISPLAY_STRETCH, ID_DISPLAY_TILE, ID_DISPLAY_STRETCH, MF_BYCOMMAND);
}


void CRemoteDesktopWnd::OnDisplayTile()
{
	// TODO:  在此添加命令处理程序代码
	m_DisplayMode = DISPLAY_TILE;

	CMenu*pMenu = m_Menu.GetSubMenu(0);
	EnableScrollBarCtrl(SB_VERT, 1);
	EnableScrollBarCtrl(SB_HORZ, 1);
	//
	//设置滚动条范围
	CRect ClientRect;
	GetClientRect(ClientRect);

	SCROLLINFO si;
	si.cbSize = sizeof(si);

	m_OrgPt.x = m_OrgPt.y = 0;
	//水平
	si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
	si.nPos = 0;
	si.nMin = 0;

	//
	si.nMax = m_dwDeskWidth;
	si.nPage = ClientRect.right;
	SetScrollInfo(SB_HORZ, &si);

	//
	si.nMax = m_dwDeskHeight;
	si.nPage = ClientRect.bottom;
	SetScrollInfo(SB_VERT, &si);

	pMenu->CheckMenuRadioItem(ID_DISPLAY_STRETCH, ID_DISPLAY_TILE, ID_DISPLAY_TILE, MF_BYCOMMAND);
}


void CRemoteDesktopWnd::OnCaptureMouse()
{
	m_dwCaptureFlags = (m_dwCaptureFlags & (~CAPTURE_MOUSE)) | (CAPTURE_MOUSE & (~(m_dwCaptureFlags & CAPTURE_MOUSE)));

	m_pHandler->SetCaptureFlag(
		REMOTEDESKTOP_FLAG_CAPTURE_MOUSE | ((m_dwCaptureFlags&CAPTURE_MOUSE)? 0x80000000 : 0));
}

void CRemoteDesktopWnd::OnCaptureTransparentwindow()
{
	m_dwCaptureFlags = (m_dwCaptureFlags & (~CAPTURE_TRANSPARENT)) | (CAPTURE_TRANSPARENT & (~(m_dwCaptureFlags & CAPTURE_TRANSPARENT)));
	m_pHandler->SetCaptureFlag(
		REMOTEDESKTOP_FLAG_CAPTURE_TRANSPARENT | ((m_dwCaptureFlags&CAPTURE_TRANSPARENT) ? 0x80000000 : 0));

}

void CRemoteDesktopWnd::OnControlKeyboard()
{
	m_ControlFlags = (m_ControlFlags & (~CONTROL_KEYBOARD)) | (CONTROL_KEYBOARD & (~(m_ControlFlags & CONTROL_KEYBOARD)));
}


void CRemoteDesktopWnd::OnControlMouse()
{
	m_ControlFlags = (m_ControlFlags & (~CONTROL_MOUSE)) | (CONTROL_MOUSE & (~(m_ControlFlags & CONTROL_MOUSE)));
}

void CRemoteDesktopWnd::OnUpdateControlMouse(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(((m_ControlFlags&CONTROL_MOUSE) != 0));
}

void CRemoteDesktopWnd::OnUpdateControlKeyboard(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(((m_ControlFlags&CONTROL_KEYBOARD) != 0));
}


void CRemoteDesktopWnd::OnUpdateCaptureMouse(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_dwCaptureFlags & CAPTURE_MOUSE);
}


void CRemoteDesktopWnd::OnUpdateCaptureTransparentwindow(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_dwCaptureFlags & CAPTURE_TRANSPARENT);
}

LRESULT CRemoteDesktopWnd::OnError(WPARAM wParam, LPARAM lParam)
{
	TCHAR * Tips = (TCHAR*)wParam;
	MessageBox(Tips, TEXT("Error"),MB_OK|MB_ICONINFORMATION);
	return 0;
}


/*
		滚动条的知识:
			min,Max代表滚动条的长度
			nPage 代表滑块的长度.
			滑块的起始位置是 nPos.

			nPos <= Max - nPage.

		这里直接改为 Page 为客户区大小,Max为远程桌面大小,
		这样滚动条的Pos 的范围就是 0 - （远程桌面大小 - 客户区大小);
*/


LRESULT CRemoteDesktopWnd::OnDesktopSize(WPARAM wParam, LPARAM lParam)
{
	m_dwDeskWidth = wParam;
	m_dwDeskHeight = lParam;
	
	CRect WndRect;
	CRect ClientRect;

	GetWindowRect(WndRect);
	GetClientRect(ClientRect);

	if (m_DisplayMode & DISPLAY_TILE)
	{	
		//平铺模式.
		//重新调整位置..

		SCROLLINFO si;
		si.cbSize = sizeof(si);

		m_OrgPt.x = m_OrgPt.y = 0;
		//水平
		si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
		si.nPos = 0;
		si.nMin = 0;

		si.nMax = m_dwDeskWidth;
		si.nPage = ClientRect.right;
		SetScrollInfo(SB_HORZ, &si);

		//
		si.nMax = m_dwDeskHeight;
		si.nPage = ClientRect.bottom;
		SetScrollInfo(SB_VERT, &si);
	}

	ClientToScreen(ClientRect);
	//
	m_dwMaxHeight = m_dwDeskHeight + ClientRect.top - WndRect.top + WndRect.bottom - ClientRect.bottom;
	m_dwMaxWidth = m_dwDeskWidth + ClientRect.left - WndRect.left + WndRect.right - ClientRect.right;
	//
	GetClientRect(ClientRect);

	int newWidth = min(m_dwDeskWidth, ClientRect.Width());
	int newHeight = min(m_dwDeskHeight, ClientRect.Height());

	WndRect.right += newWidth - ClientRect.Width();
	WndRect.bottom += newHeight - ClientRect.Height();

	MoveWindow(WndRect);
	return 0;
}

LRESULT CRemoteDesktopWnd::OnDraw(WPARAM wParam, LPARAM lParam)
{
	LPVOID * Image	= (LPVOID*)wParam;
	HDC     hMemDC	= (HDC)	   Image[0];
	BITMAP* pBitmap	= (BITMAP*)Image[1];
	HCURSOR hCursor = (HCURSOR)lParam;

	RECT rect = { 0 };

	GetClientRect(&rect);
	if (m_DisplayMode & DISPLAY_STRETCH)
	{
		::StretchBlt(
			m_hDC, 
			0, 
			0, 
			rect.right, 
			rect.bottom, 
			hMemDC, 
			0, 
			0, 
			pBitmap->bmWidth, 
			pBitmap->bmHeight, 
			SRCCOPY
		);
	}
	else if (m_DisplayMode & DISPLAY_TILE){
		::BitBlt(
			m_hDC,
			-m_OrgPt.x,
			-m_OrgPt.y, 
			m_dwDeskWidth,
			m_dwDeskHeight,
			hMemDC,
			0,
			0, 
			SRCCOPY
		);
	}
	else if (m_DisplayMode & DISPLAY_FULLSCREEN)
	{
		if (m_bFullScreenStretchMode)
		{	
			::StretchBlt(
				m_hDC,
				0, 
				0, 
				rect.right,
				rect.bottom, 
				hMemDC,
				0, 
				0, 
				pBitmap->bmWidth,
				pBitmap->bmHeight,
				SRCCOPY
			);
		}
		else
		{
			::BitBlt(
				m_hDC, 
				m_FullScreenDrawOrg.x, 
				m_FullScreenDrawOrg.y, 
				m_dwDeskWidth, 
				m_dwDeskHeight, 
				hMemDC, 
				0, 
				0, 
				SRCCOPY
			);
		}
	}

	m_dwFps++;

	//update fps
	if ((GetTickCount() - m_dwLastTime) >= 1000)
	{
		CString text;
		//显示FPS
		text.Format(TEXT("%s - [Fps: %d]"), m_Title.GetBuffer(), m_dwFps);
		SetWindowText(text);
		//刷新
		m_dwLastTime = GetTickCount();
		m_dwFps = 0;
	}

	//设置光标.
	if ((m_dwCaptureFlags & CONTROL_MOUSE) &&
		m_LastCursor != hCursor)
	{
		SetCursor(hCursor);
		m_LastCursor = hCursor;
	}
	return 0;
}


void CRemoteDesktopWnd::OnSize(UINT nType, int cx, int cy)
{
	// TODO:  在此处添加消息处理程序代码
	if (m_DisplayMode & DISPLAY_TILE && m_dwMaxHeight != 0 &&m_dwMaxWidth !=0 )
	{
		//限制大小.
		if (cx > m_dwDeskWidth || cy > m_dwDeskHeight)
		{
			CRect rect;
			GetWindowRect(rect);
			if (cx > m_dwDeskWidth)
				rect.right = rect.left + m_dwMaxWidth;
			if (cy > m_dwDeskHeight)
				rect.bottom = rect.top + m_dwMaxHeight;
			MoveWindow(rect, 1);
			return CFrameWnd::OnSize(nType, cx, cy);
		}

		//,调整滚动条nPage大小.
		SCROLLINFO si = { sizeof(si) };
		si.fMask = SIF_PAGE | SIF_RANGE;
		//
		si.nPage = cx;
		si.nMin = 0, si.nMax = m_dwDeskWidth;
		SetScrollInfo(SB_HORZ, &si);

		si.nPage = cy;
		si.nMin = 0, si.nMax = m_dwDeskHeight;
		SetScrollInfo(SB_VERT, &si);
		
		//设置绘制原点.
		m_OrgPt.x = GetScrollPos(SB_HORZ);
		m_OrgPt.y = GetScrollPos(SB_VERT);
	}
	CFrameWnd::OnSize(nType, cx, cy);
}


BOOL CRemoteDesktopWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO:  在此添加专用代码和/或调用基类
	cs.style |= WS_VSCROLL;
	cs.style |= WS_HSCROLL;

	if (CFrameWnd::PreCreateWindow(cs))
	{
		static BOOL bRegistered = FALSE;

		if (!bRegistered)
		{
			WNDCLASS wndclass = { 0 };
			GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndclass);
			//取消水平大小和竖直大小变化时的重绘.没用OnPaint,会白屏.
			wndclass.style &= ~(CS_HREDRAW |CS_VREDRAW );
			wndclass.lpszClassName = TEXT("RemoteDesktopWndClass");
			wndclass.hCursor = NULL;
			bRegistered = AfxRegisterClass(&wndclass);
		}
		if (!bRegistered)
			return FALSE;
		cs.lpszClass = TEXT("RemoteDesktopWndClass");
		return TRUE;
	}
	return FALSE;
}


void CRemoteDesktopWnd::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	if (m_DisplayMode & DISPLAY_TILE){
		lpMMI->ptMaxSize.x = m_dwMaxWidth;
		lpMMI->ptMaxSize.y = m_dwMaxHeight;
	}
	else if (m_DisplayMode & DISPLAY_FULLSCREEN){

		//需要这些代码才能全屏，至于为啥我也不知道，反正它就是全屏了....
		lpMMI->ptMaxSize.x = m_FullScreen.Width();
		lpMMI->ptMaxSize.y = m_FullScreen.Height();

		lpMMI->ptMaxPosition.x = m_FullScreen.left;
		lpMMI->ptMaxPosition.y = m_FullScreen.top;

		lpMMI->ptMaxTrackSize.x = m_FullScreen.Width();
		lpMMI->ptMaxTrackSize.y = m_FullScreen.Height();
	}


	CFrameWnd::OnGetMinMaxInfo(lpMMI);
}


void CRemoteDesktopWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CRect rect;
	SCROLLINFO si = { 0 };
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_HORZ, &si);
	GetClientRect(rect);

	int sub_delta = si.nPos < SCROLL_UINT ? si.nPos : SCROLL_UINT;
	int add_delta = (si.nMax - si.nPage - si.nPos) > SCROLL_UINT ? SCROLL_UINT : (si.nMax - si.nPage - si.nPos);

	switch (nSBCode)
	{
	case SB_LINEUP:
		si.nPos -= sub_delta;
		break;
	case SB_LINEDOWN:
		si.nPos += add_delta;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	default:
		break;
	}

	m_OrgPt.x = si.nPos;
	SetScrollPos(SB_HORZ, si.nPos);
	CFrameWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CRemoteDesktopWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CRect rect;
	SCROLLINFO si = { sizeof(si) };
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);

	GetClientRect(rect);

	int sub_delta = si.nPos < SCROLL_UINT ? si.nPos : SCROLL_UINT;
	int add_delta = (si.nMax - si.nPage - si.nPos) > SCROLL_UINT ? SCROLL_UINT : (si.nMax - si.nPage - si.nPos);

	switch (nSBCode)
	{
	case SB_LINEUP:
		si.nPos -= sub_delta;
		break;
	case SB_LINEDOWN:
		si.nPos += add_delta;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	default:
		break;
	}

	m_OrgPt.y = si.nPos;
	SetScrollPos(SB_VERT, si.nPos);

	CFrameWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CRemoteDesktopWnd::OnSetFocus(CWnd* pOldWnd)
{
	CFrameWnd::OnSetFocus(pOldWnd);

	// TODO:  在此处添加消息处理程序代码
	hTopWindow = m_hWnd;
}


void CRemoteDesktopWnd::OnMaxfps10()
{
	m_dwMaxpFps = 10;
	GetMenu()->GetSubMenu(0)->GetSubMenu(2)->
		CheckMenuRadioItem(
		ID_MAXFPS_10, 
		ID_MAXFPS_NOLIMIT, 
		ID_MAXFPS_10, 
		MF_BYCOMMAND);

	m_pHandler->StartRDP(m_dwMaxpFps, m_dwQuality);
		
}


void CRemoteDesktopWnd::OnMaxfps20()
{
	m_dwMaxpFps = 20;
	GetMenu()->GetSubMenu(0)->GetSubMenu(2)->
		CheckMenuRadioItem(
		ID_MAXFPS_10,
		ID_MAXFPS_NOLIMIT, 
		ID_MAXFPS_20,
		MF_BYCOMMAND);

	m_pHandler->StartRDP(m_dwMaxpFps, m_dwQuality);
	
		
}


void CRemoteDesktopWnd::OnMaxfps30()
{
	m_dwMaxpFps = 30;
	GetMenu()->GetSubMenu(0)->GetSubMenu(2)->
		CheckMenuRadioItem(
		ID_MAXFPS_10, 
		ID_MAXFPS_NOLIMIT, 
		ID_MAXFPS_30,
		MF_BYCOMMAND);

	m_pHandler->StartRDP(m_dwMaxpFps, m_dwQuality);

}


void CRemoteDesktopWnd::OnMaxfpsNolimit()
{
	m_dwMaxpFps = 60;
	GetMenu()->GetSubMenu(0)->GetSubMenu(2)->
		CheckMenuRadioItem(
		ID_MAXFPS_10, 
		ID_MAXFPS_NOLIMIT, 
		ID_MAXFPS_NOLIMIT, 
		MF_BYCOMMAND);

	m_pHandler->StartRDP(m_dwMaxpFps, m_dwQuality);
	
}


LRESULT CRemoteDesktopWnd::OnSetClipbdText(WPARAM wParam, LPARAM lParam)
{
	//设置剪切板数据
	if (OpenClipboard())
	{
		UINT32 cbBuff;
		EmptyClipboard();
		m_SetClipbdText = (TCHAR*)wParam;
		cbBuff = sizeof(TCHAR) * (m_SetClipbdText.GetLength() + 1);

		HGLOBAL hData = GlobalAlloc(GHND | GMEM_SHARE, cbBuff);

		if (hData)
		{
			TCHAR*pBuff = (TCHAR*)GlobalLock(hData);
			lstrcpy(pBuff, m_SetClipbdText);
			GlobalUnlock(hData);
			//
			SetClipboardData(CF_UNICODETEXT, hData);
		}
		CloseClipboard();
	}
	return 0;
}

void CRemoteDesktopWnd::OnChangeCbChain(HWND hWndRemove, HWND hWndAfter)
{
	CFrameWnd::OnChangeCbChain(hWndRemove, hWndAfter);
	// TODO:  在此处添加消息处理程序代码
	if (hWndRemove == m_hNextViewer){
		m_hNextViewer = hWndAfter;
	}
	else if (m_hNextViewer){
		::SendMessage(m_hNextViewer, WM_CHANGECBCHAIN,
			(WPARAM)hWndRemove, (LPARAM)hWndAfter);
	}
	//
}


void CRemoteDesktopWnd::OnDrawClipboard()
{
	CFrameWnd::OnDrawClipboard();
	// TODO:  在此处添加消息处理程序代码
	if (OpenClipboard()){
		HGLOBAL hData = GetClipboardData(CF_UNICODETEXT);
		if (hData)
		{
			TCHAR *szText = (TCHAR*)GlobalLock(hData);
			if (szText && m_SetClipbdText != szText)
			{
				//数据改变了,通知对方改变数据
				m_pHandler->SetClipboardText(szText);
				//MessageBox(L"通知对方改数据了", L"Tips", MB_OK);
			}
			GlobalUnlock(hData);
		}
		CloseClipboard();
	}
	//通知下一个Viewer.
	if (m_hNextViewer){
		::SendMessage(m_hNextViewer, WM_DRAWCLIPBOARD, 0, 0);
	}
}


void CRemoteDesktopWnd::OnDisplayFullscreen()
{
	m_OldDisplayMode = m_DisplayMode;
	m_DisplayMode = DISPLAY_FULLSCREEN;
	// 全屏显示
	GetWindowPlacement(&m_old);
	
	HDC hdc = ::GetDC(::GetDesktopWindow());
	//获取显示器分辨率	
	DWORD dwHeight = GetDeviceCaps(hdc, VERTRES);
	DWORD dwWidth = GetDeviceCaps(hdc, HORZRES);
	::ReleaseDC(::GetDesktopWindow(), hdc);

	CRect WndRect, ClientRect;
	EnableScrollBarCtrl(SB_VERT, 0);			//Hide Scroll Bars.
	EnableScrollBarCtrl(SB_HORZ, 0);

	GetWindowRect(WndRect);
	GetClientRect(ClientRect);
	ClientToScreen(ClientRect);
	
	//
	m_FullScreen.left = WndRect.left - ClientRect.left;
	m_FullScreen.top = WndRect.top - ClientRect.top;
	m_FullScreen.right = WndRect.right - ClientRect.right + dwWidth;
	m_FullScreen.bottom = WndRect.bottom - ClientRect.bottom + dwHeight;

	//
	WINDOWPLACEMENT wp = { 0 };
	wp.length = sizeof(wp);
	wp.flags = 0;
	wp.showCmd = SW_SHOWNORMAL;
	wp.rcNormalPosition = m_FullScreen;

	SetWindowPlacement(&wp);
	
	/*自定义显示方式:
			1.如果远程桌面小于当前显示器的分辨率，那么就居中显示远程桌面
			2.否则拉伸显示远程桌面.
	*/
	GetClientRect(ClientRect);
	FillRect(m_hDC, ClientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	if (m_dwDeskWidth < dwWidth && m_dwDeskHeight < dwHeight){
		m_FullScreenDrawOrg.x = (dwWidth - m_dwDeskWidth) / 2;
		m_FullScreenDrawOrg.y = (dwHeight - m_dwDeskHeight) / 2;
		//
		m_bFullScreenStretchMode = FALSE;
	}
	else{
		m_FullScreenDrawOrg.x = 0;
		m_FullScreenDrawOrg.y = 0;

		m_bFullScreenStretchMode = TRUE;
	}
}


LRESULT CRemoteDesktopWnd::OnScreenShot(WPARAM wParam,LPARAM lParam){
	char * Buffer = (char*)wParam;
	DWORD dwSize = lParam;

	if (Buffer == NULL)
	{
		return 0;
	}

	CString FileName;
	CTime Time = CTime::GetTickCount();
	FileName.Format(TEXT("\\%s.bmp"), Time.Format("%Y-%m-%d_%H_%M_%S").GetBuffer());

	CMainFrame * pMainWnd = (CMainFrame*)AfxGetMainWnd();
	CString value    = pMainWnd->Config().GetConfig(TEXT("remote_desktop"), TEXT("screenshot_save_path"));
	CString SavePath;

	SavePath += value;
	SavePath += FileName;

	if (SavePath[1] != ':')
	{
		CString csCurrentDir;
		csCurrentDir.Preallocate(MAX_PATH);
		GetCurrentDirectory(MAX_PATH, csCurrentDir.GetBuffer());
		SavePath = csCurrentDir + "\\" + SavePath;
	}

	MakesureDirExist(SavePath, TRUE);

	HANDLE hFile = CreateFile(
		SavePath, 
		GENERIC_WRITE,
		NULL,
		NULL, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	CString err;

	if (hFile == INVALID_HANDLE_VALUE)
	{
		err.Format(TEXT("CreateFile failed with error: %d"), GetLastError());
	}

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwWrite = 0;
		WriteFile(hFile, Buffer, dwSize, &dwWrite, NULL);
		CloseHandle(hFile);

		err.Format(TEXT("Image has been save to %s"), SavePath);
	}
	MessageBox(err, TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
	return 0;
}

void CRemoteDesktopWnd::OnOtherScreenshot()
{
	m_pHandler->ScreenShot();
}



BOOL CRemoteDesktopWnd::PreTranslateMessage(MSG* pMsg)
{
	UINT message = pMsg->message;
	WPARAM wParam = pMsg->wParam;
	LPARAM lParam = pMsg->lParam;

	// TODO:  在此添加专用代码和/或调用基类
	if ((message == WM_KEYDOWN) && (wParam == VK_ESCAPE) &&
		(m_DisplayMode &DISPLAY_FULLSCREEN)
		){
		//退出全屏.
		m_DisplayMode = m_OldDisplayMode;
		SetWindowPlacement(&m_old);
		return TRUE;
	}

	// TODO:  在此添加专用代码和/或调用基类
	if ((!(m_ControlFlags&CONTROL_MOUSE)) &&
		(!(m_ControlFlags&CONTROL_KEYBOARD)))
		return FALSE;

	CRemoteDesktopSrv::CtrlParam Param = { 0 };

	if ((m_ControlFlags&CONTROL_MOUSE) && 
		message >= WM_MOUSEMOVE && message <= WM_MOUSEWHEEL)
	{
		DWORD dwX, dwY;
		dwX = LOWORD(lParam);
		dwY = HIWORD(lParam);

		if (m_DisplayMode & DISPLAY_STRETCH)
		{
			RECT rect;
			GetClientRect(&rect);
			dwX = (DWORD)(dwX * 65535.0 / rect.right);
			dwY = (DWORD)(dwY * 65535.0 / rect.bottom);
		}
		else if (m_DisplayMode & DISPLAY_TILE)
		{
			dwX += (DWORD)(m_OrgPt.x);
			dwY += (DWORD)(m_OrgPt.y);
			dwX = (DWORD)(dwX * 65535.0 / m_dwDeskWidth);
			dwY = (DWORD)(dwY * 65535.0 / m_dwDeskHeight);
		}
		else if (m_DisplayMode & DISPLAY_FULLSCREEN){
			//
			if (m_bFullScreenStretchMode){
				RECT rect;
				GetClientRect(&rect);
				dwX = (DWORD)(dwX * 65535.0 / rect.right);
				dwY = (DWORD)(dwY * 65535.0 / rect.bottom);
			}
			else {
				//
				if (dwX < m_FullScreenDrawOrg.x || dwX >(m_FullScreenDrawOrg.x + m_dwDeskWidth) ||
					dwY < m_FullScreenDrawOrg.y || dwY >(m_FullScreenDrawOrg.y + m_dwDeskHeight)){
					return FALSE;
				}
				//
				dwX = (DWORD)((dwX - m_FullScreenDrawOrg.x) * 65535.0 / m_dwDeskWidth);
				dwY = (DWORD)((dwY - m_FullScreenDrawOrg.y) * 65535.0 / m_dwDeskHeight);
			}
		}

		Param.dwType = message;
		Param.Param.dwCoor = MAKEWPARAM(dwX, dwY);		//坐标.

		if (message == WM_MOUSEWHEEL)
			Param.dwExtraData = (SHORT)(HIWORD(wParam));
		
		m_pHandler->Control(&Param);
		return TRUE;
	}

	if ((m_ControlFlags&CONTROL_KEYBOARD) && 
		(message == WM_KEYDOWN || message == WM_KEYUP))
	{
		Param.dwType = message;				//消息类型
		Param.Param.VkCode = wParam;		//vkCode
		m_pHandler->Control(&Param);		//
		return TRUE;
	}
	return CFrameWnd::PreTranslateMessage(pMsg);
}

void CRemoteDesktopWnd::OnQualityLow()
{
	m_dwQuality = QUALITY_LOW;
	GetMenu()->GetSubMenu(0)->GetSubMenu(3)->
		CheckMenuRadioItem(
		ID_QUALITY_LOW, 
		ID_QUALITY_HIGH, 
		ID_QUALITY_LOW,
		MF_BYCOMMAND);


	m_pHandler->StartRDP(m_dwMaxpFps, m_dwQuality);
	
}


void CRemoteDesktopWnd::OnQualityHigh()
{
	m_dwQuality = QUALITY_HIGH;
	GetMenu()->GetSubMenu(0)->GetSubMenu(3)->
		CheckMenuRadioItem(
		ID_QUALITY_LOW, 
		ID_QUALITY_HIGH,
		ID_QUALITY_HIGH,
		MF_BYCOMMAND);

	m_pHandler->StartRDP(m_dwMaxpFps, m_dwQuality);
	
}
