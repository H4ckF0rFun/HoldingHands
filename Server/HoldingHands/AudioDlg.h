#pragma once

#include "resource.h"
#include "afxwin.h"
// CAudioDlg 对话框

class CAudioSrv;

class CAudioDlg : public CDialog
{
	DECLARE_DYNAMIC(CAudioDlg)

public:
	CAudioDlg(CAudioSrv*pHandler,CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CAudioDlg();

// 对话框数据
	enum { IDD = IDD_AUDIODLG };

	CAudioSrv*	m_pHandler;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

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
