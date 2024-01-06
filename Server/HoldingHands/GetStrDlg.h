#pragma once
#include "afxwin.h"


// CEditCommentDlg 对话框

class CGetStrDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGetStrDlg)

public:
	CGetStrDlg(const CString & Title,CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CGetStrDlg();

// 对话框数据
	enum { IDD = IDD_GETSTR_DLG };

protected:
	CString m_Title;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
//	CEdit m_Comment;
	CString m_str;

	virtual BOOL OnInitDialog();
};
