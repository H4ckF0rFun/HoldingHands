#pragma once
#include "afxwin.h"


// CEditCommentDlg �Ի���

class CEditCommentDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CEditCommentDlg)

public:
	CEditCommentDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CEditCommentDlg();

// �Ի�������
	enum { IDD = IDD_EDITCMT_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
//	CEdit m_Comment;
	CString m_Comment;
	virtual BOOL OnInitDialog();
};
