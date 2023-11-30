#pragma once
#include "afxwin.h"
#include "resource.h"



// CKeybdLogDlg �Ի���
class CKeybdLogSrv;

class CKeybdLogDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CKeybdLogDlg)

public:
	CKeybdLogDlg(CKeybdLogSrv*pHandler,CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CKeybdLogDlg();

// �Ի�������
	enum { IDD = IDD_KBD_LOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	

	DECLARE_MESSAGE_MAP()
public:
	BOOL			m_DestroyAfterDisconnect;
	CKeybdLogSrv*	m_pHandler;
	DWORD			m_dwTimerId;
	CEdit m_Log;
	DWORD m_Interval;
	CButton m_OfflineRecord;

	afx_msg LRESULT OnLogData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLogInit(WPARAM wParam, LPARAM lParam);

	afx_msg void OnClose();
	virtual BOOL OnInitDialog();
	
	afx_msg void OnBnClickedOk2();
	afx_msg void OnBnClickedOk();
	virtual void OnCancel();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedButton1();
	virtual void PostNcDestroy();
	virtual void OnOK();
};
