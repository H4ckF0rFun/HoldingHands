#pragma once
#include "afxwin.h"


// CEditCommentDlg �Ի���

class CGetStrDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGetStrDlg)

public:
	CGetStrDlg(const CString & Title,CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CGetStrDlg();

// �Ի�������
	enum { IDD = IDD_GETSTR_DLG };

protected:
	CString m_Title;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
//	CEdit m_Comment;
	CString m_str;

	virtual BOOL OnInitDialog();
};
