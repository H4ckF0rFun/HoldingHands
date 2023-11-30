#pragma once
#include "afxcmn.h"


// CSocksProxyAddrDlg 对话框

class CSocksProxyAddrDlg : public CDialog
{
	DECLARE_DYNAMIC(CSocksProxyAddrDlg)

public:
	CSocksProxyAddrDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSocksProxyAddrDlg();

// 对话框数据
	enum { IDD = IDD_SOCKS_ADDR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	DWORD m_Port;
	DWORD m_Addr;
	DWORD m_UDPAssociateAddress;
};
