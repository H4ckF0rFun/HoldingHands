// EditCommentDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "EditCommentDlg.h"
#include "afxdialogex.h"
#include "resource.h"

// CEditCommentDlg �Ի���

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


// CEditCommentDlg ��Ϣ�������


void CEditCommentDlg::OnBnClickedOk()
{
	UpdateData();
	if (!m_Comment.GetLength())
		return;
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	CDialogEx::OnOK();
}


BOOL CEditCommentDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowText(TEXT("EditComment"));
	GetDlgItem(IDC_EDIT1)->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
}
