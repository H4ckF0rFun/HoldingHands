#pragma once
#include "resource.h"

// CBuildDlg �Ի���

class CBuildDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBuildDlg)

public:
	CBuildDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CBuildDlg();

// �Ի�������
	enum { IDD = IDD_BUILD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CString m_ServerIP;
	CString m_Port;
	afx_msg void OnBnClickedOk();
};
