// SettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "SettingDlg.h"
#include "afxdialogex.h"
#include "MainFrm.h"

// CSettingDlg 对话框

IMPLEMENT_DYNAMIC(CSettingDlg, CDialogEx)

CSettingDlg::CSettingDlg(CConfig&config,CWnd* pParent /*=NULL*/)
	: CDialogEx(CSettingDlg::IDD, pParent)
	, m_config(config)
{

}

CSettingDlg::~CSettingDlg()
{
}

void CSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSettingDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSettingDlg 消息处理程序
void CSettingDlg::OnBnClickedOk()
{
	CDialogEx::OnOK();
}

