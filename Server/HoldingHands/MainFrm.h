
// MainFrm.h : CMainFrame 类的接口
//

#pragma once
#include "Config.h"

#include "OutputWnd.h"

#define VIEW_SHOW_LOG				0x1
#define VIEW_SHOW_STATUS_BAR		0x2

#define SRV_STATU_STARTING	0			//正在开启
#define SRV_STATU_STARTED	1			//已经开启
#define SRV_STATU_STOPPING	2			//正在关闭
#define SRV_STATU_STOPPED	3			//已经关闭


class CLogCtrl;

class CIOCP;
class CServer;

class CMainFrame : public CFrameWndEx
{
public:
	CMainFrame();

	
protected: 
	DECLARE_DYNCREATE(CMainFrame)

// 特性
public:

// 操作
public:

// 重写
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
// 实现
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:  // 控件条嵌入成员
	CMFCStatusBar     m_wndStatusBar;
	COutputWnd        m_wndOutput;

	DWORD			  m_View;
	DWORD			  m_ServerStatu;
	////
	CIOCP*			  m_Iocp;
	CServer *	      m_pServer;
	
	CConfig			  m_config;
	//
	unsigned int	  m_listenPort;
// 生成的消息映射函数
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	DECLARE_MESSAGE_MAP()

	//void Log(CString text);
public:
	CConfig& Config(){
		return m_config;
	}

	CLogCtrl* GetLogCtrl() { return m_wndOutput.GetLogCtrl(); };
	CMFCStatusBar * GetStatusBar() { return &m_wndStatusBar;  };

	afx_msg void OnPaint();

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMainStartserver();
	afx_msg void OnUpdateMainStartserver(CCmdUI *pCmdUI);

	afx_msg void OnUpdateStatuBar();

	////服务器开启和关闭后会通知该窗口
	//afx_msg LRESULT OnSrvStarted(WPARAM wResult, LPARAM lNoUsed);
	//afx_msg LRESULT OnSrvStopped(WPARAM wNoUsed, LPARAM lNoUsed);

	//当有socket连接的时候通过该函数来初始化Handler
	afx_msg LRESULT OnClientLogin(WPARAM wParam,LPARAM lParam);

	afx_msg void OnClose();
	afx_msg void OnMainExit();
	afx_msg void OnUpdateMainExit(CCmdUI *pCmdUI);
	afx_msg void OnMainBuild();
	afx_msg void OnMainSettings();

	afx_msg void OnSize(UINT nType, int cx, int cy);



	afx_msg void OnOperationProcessmanager();
	afx_msg void OnRemotedesktopDxgi();
	afx_msg void OnRemotedesktopGdi();
	afx_msg void OnUpdateViewLogview(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewStatusbar(CCmdUI *pCmdUI);
	afx_msg void OnViewLogview();
	afx_msg void OnViewStatusbar();
};


