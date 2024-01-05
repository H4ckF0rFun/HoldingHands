#pragma once
#include "afxwin.h"
#include "resource.h"
class CRemoteDesktopSrv;


#define SCROLL_UINT		15

#define CONTROL_MOUSE		(0x1)
#define CONTROL_KEYBOARD	(0x2)

#define DISPLAY_FULLSCREEN  0x4
#define DISPLAY_STRETCH		0x8
#define DISPLAY_TILE		0x10

//Message...


class CRemoteDesktopWnd :
	public CFrameWnd
{
public:
	//

	CRemoteDesktopSrv*m_pHandler;
	//FPS 
	DWORD	m_dwLastTime;
	DWORD	m_dwFps;
	CString m_Title;
	//
	DWORD	m_dwDeskWidth;		//客户区最大尺寸
	DWORD	m_dwDeskHeight;
							
	DWORD	m_dwMaxHeight;		//窗口最大尺寸
	DWORD	m_dwMaxWidth;

	HDC		m_hDC;
	POINT	m_OrgPt;			//滚动条位置.

	HCURSOR m_LastCursor;

	// CONTROL
	DWORD   m_ControlFlags;		//

	DWORD	m_dwMaxpFps;
	DWORD	m_dwQuality;

	CMenu	m_Menu;
	CString m_SetClipbdText;		//自己设置的内容;
	HWND	m_hNextViewer;
	//
	DWORD	m_DisplayMode;			//全屏需要的信息
	DWORD	m_OldDisplayMode;
	WINDOWPLACEMENT m_old;
	CRect	m_FullScreen;
	CPoint	m_FullScreenDrawOrg;	//
	BOOL	m_bFullScreenStretchMode;

	//
	int     m_nMonitors;
	int     m_CurrentMonitor;

	CRemoteDesktopWnd(CRemoteDesktopSrv*pHandler);
	~CRemoteDesktopWnd();
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDisplayStretch();
	afx_msg void OnDisplayTile();
	afx_msg void OnCaptureMouse();
	afx_msg void OnControlKeyboard();
	afx_msg void OnControlMouse();
	afx_msg void OnCaptureTransparentwindow();
	afx_msg void OnUpdateControlMouse(CCmdUI *pCmdUI);
	afx_msg void OnUpdateControlKeyboard(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCaptureMouse(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCaptureTransparentwindow(CCmdUI *pCmdUI);

	void SwitchMonitor(UINT id);
	LRESULT	OnDraw(WPARAM wParam, LPARAM lParam);
	LRESULT OnError(WPARAM wParam, LPARAM lParam);
	LRESULT OnDesktopSize(WPARAM wParam, LPARAM lParam);
	LRESULT OnSetClipbdText(WPARAM wParam, LPARAM lParam);
	//LRESULT OnScreenShot(WPARAM wParam, LPARAM lParam);
	LRESULT OnGetDrawHwnd(WPARAM wParam, LPARAM lParam);
	LRESULT OnGetScreenshotSavePath(WPARAM wParam, LPARAM lParamm);
	LRESULT OnMonitorsInfo(WPARAM wParam, LPARAM lParam);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnMaxfps10();
	afx_msg void OnMaxfps20();
	afx_msg void OnMaxfps30();
	afx_msg void OnMaxfpsNolimit();
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnDrawClipboard();

	afx_msg void OnDisplayFullscreen();
	afx_msg void OnOtherScreenshot();
	virtual void PostNcDestroy();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	
	//afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnQualityLow();
	afx_msg void OnQualityHigh();
	virtual void OnUpdateFrameMenu(HMENU hMenuAlt);
};

