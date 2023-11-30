#pragma once
#include "afxwin.h"


// CEditCommentDlg 对话框

class CEditCommentDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CEditCommentDlg)

public:
	CEditCommentDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CEditCommentDlg();

// 对话框数据
	enum { IDD = IDD_EDITCMT_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
//	CEdit m_Comment;
	CString m_Comment;
	virtual BOOL OnInitDialog();
};
