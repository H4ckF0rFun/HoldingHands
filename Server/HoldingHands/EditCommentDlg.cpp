// EditCommentDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "EditCommentDlg.h"
#include "afxdialogex.h"
#include "resource.h"

// CEditCommentDlg 对话框

IMPLEMENT_DYNAMIC(CEditCommentDlg, CDialogEx)

CEditCommentDlg::CEditCommentDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CEditCommentDlg::IDD, pParent)
	, m_Comment(_T(""))
{
}

CEditCommentDlg::~CEditCommentDlg()
{
}

void CEditCommentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//  DDX_Control(pDX, IDC_EDIT1, m_Comment);
	DDX_Text(pDX, IDC_EDIT1, m_Comment);
	DDV_MaxChars(pDX, m_Comment, 200);
}


BEGIN_MESSAGE_MAP(CEditCommentDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CEditCommentDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CEditCommentDlg 消息处理程序


void CEditCommentDlg::OnBnClickedOk()
{
	UpdateData();
	if (!m_Comment.GetLength())
		return;
	// TODO:  在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


BOOL CEditCommentDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowText(TEXT("EditComment"));
	GetDlgItem(IDC_EDIT1)->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
}
