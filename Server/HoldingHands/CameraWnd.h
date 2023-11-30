#pragma once
#include "afxwin.h"



typedef struct tagVideoSize{
	DWORD Width;
	DWORD Height;
}VideoSize;

class CCameraSrv;
class CCameraWnd :
	public CFrameWnd
{
public:

	HDC			m_hdc;
	DWORD		m_dwFps;
	DWORD		m_dwLastTime;

	//Client Size
	DWORD		m_dwWidth;
	DWORD		m_dwHeight;

	//Video Size:
	DWORD		m_VideoHeight;
	DWORD		m_VideoWidth;

	CPoint		m_Org;
	CString		 m_Title;


	CCameraSrv*	m_pHandler;

	//Device Info..
	CStringArray m_deviceName;
	CDWordArray	 m_sizeToDevice;
	CArray<VideoSize> m_VideoSize;
	DWORD		 m_SizeCount;
	DWORD		 m_DeviceCount;
	DWORD		 m_CurrentSizeID;

	CCameraWnd(CCameraSrv*pHandler);
	~CCameraWnd();
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);


	afx_msg LRESULT OnDeviceList(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClose();
	virtual void PostNcDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	afx_msg LRESULT OnVideoSize(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFrame(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);

	void OnScreenShot();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
};

