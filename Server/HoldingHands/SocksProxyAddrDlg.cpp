// SocksProxyAddrDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "SocksProxyAddrDlg.h"
#include "afxdialogex.h"


// CSocksProxyAddrDlg 对话框

IMPLEMENT_DYNAMIC(CSocksProxyAddrDlg, CDialog)

CSocksProxyAddrDlg::CSocksProxyAddrDlg(CWnd* pParent /*=NULL*/)
		: CDialog(CSocksProxyAddrDlg::IDD, pParent)
	, m_Port(1080)
	, m_Addr(0)
	, m_UDPAssociateAddress(0x7f000001)
{

}

CSocksProxyAddrDlg::~CSocksProxyAddrDlg()
{
}

void CSocksProxyAddrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Port);
	DDX_IPAddress(pDX, IDC_IPADDRESS1, m_Addr);
	DDX_IPAddress(pDX, IDC_IPADDRESS2, m_UDPAssociateAddress);
}


BEGIN_MESSAGE_MAP(CSocksProxyAddrDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CSocksProxyAddrDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSocksProxyAddrDlg 消息处理程序


void CSocksProxyAddrDlg::OnBnClickedOk()
{
	UpdateData();
	CDialog::OnOK();
}
