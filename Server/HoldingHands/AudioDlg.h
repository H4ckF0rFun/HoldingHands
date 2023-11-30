#pragma once

#include "resource.h"
#include "afxwin.h"
// CAudioDlg �Ի���

class CAudioSrv;

class CAudioDlg : public CDialog
{
	DECLARE_DYNAMIC(CAudioDlg)

public:
	CAudioDlg(CAudioSrv*pHandler,CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CAudioDlg();

// �Ի�������
	enum { IDD = IDD_AUDIODLG };

	CAudioSrv*	m_pHandler;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:


	afx_msg void OnClose();
	
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);

	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	CButton m_ListenLocal;
	afx_msg void OnBnClickedCheck1();
};
