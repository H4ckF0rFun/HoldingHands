// EditCommentDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "GetStrDlg.h"
#include "afxdialogex.h"
#include "resource.h"

// CEditCommentDlg 对话框

IMPLEMENT_DYNAMIC(CGetStrDlg, CDialogEx)

CGetStrDlg::CGetStrDlg(const CString & Title, CWnd* pParent /*=NULL*/)
	: CDialogEx(CGetStrDlg::IDD, pParent)
	, m_str(_T(""))
	, m_Title(Title)
{
}

CGetStrDlg::~CGetStrDlg()
{
}

void CGetStrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//  DDX_Control(pDX, IDC_EDIT1, m_Comment);
	DDX_Text(pDX, IDC_EDIT1, m_str);
	DDV_MaxChars(pDX, m_str, 200);
}


BEGIN_MESSAGE_MAP(CGetStrDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CGetStrDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CEditCommentDlg 消息处理程序


void CGetStrDlg::OnBnClickedOk()
{
	UpdateData();

	if (!m_str.GetLength())
		return;
	// TODO:  在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


BOOL CGetStrDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowText(m_Title);

	GetDlgItem(IDC_EDIT1)->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
}
