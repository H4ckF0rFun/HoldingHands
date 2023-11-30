#pragma once
#include "resource.h"

// CBuildDlg 对话框

class CBuildDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBuildDlg)

public:
	CBuildDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CBuildDlg();

// 对话框数据
	enum { IDD = IDD_BUILD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_ServerIP;
	CString m_Port;
	afx_msg void OnBnClickedOk();
};
