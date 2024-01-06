
// MainFrm.h : CMainFrame ��Ľӿ�
//

#pragma once
#include "Config.h"

#include "OutputWnd.h"

#define VIEW_SHOW_LOG				0x1
#define VIEW_SHOW_STATUS_BAR		0x2

#define SRV_STATU_STARTING	0			//���ڿ���
#define SRV_STATU_STARTED	1			//�Ѿ�����
#define SRV_STATU_STOPPING	2			//���ڹر�
#define SRV_STATU_STOPPED	3			//�Ѿ��ر�


class CLogCtrl;

class CIOCP;
class CServer;

class CMainFrame : public CFrameWndEx
{
public:
	CMainFrame();

	
protected: 
	DECLARE_DYNCREATE(CMainFrame)

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

	CLogCtrl* GetLogCtrl() { return m_wndOutput.GetLogCtrl(); };
	CMFCStatusBar * GetStatusBar() { return &m_wndStatusBar;  };

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


