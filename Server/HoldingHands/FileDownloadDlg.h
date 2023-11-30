#pragma once
#include "afxcmn.h"
#include "resource.h"


// CFileDownloadDlg �Ի���

class CFileDownloadSrv;

class CFileDownloadDlg : public CDialog
{
	DECLARE_DYNAMIC(CFileDownloadDlg)

public:
	CFileDownloadDlg(CFileDownloadSrv* pHandler, CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CFileDownloadDlg();

// �Ի�������
	enum { IDD = IDD_MNDD_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl		m_Progress;


	BOOL				m_DownloadFinished;
	CFileDownloadSrv*	m_pHandler;

	afx_msg LRESULT OnFileInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadResult(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParma);

	afx_msg void OnClose();
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual void OnCancel();
};
