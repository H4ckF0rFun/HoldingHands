// EditCommentDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "GetStrDlg.h"
#include "afxdialogex.h"
#include "resource.h"

// CEditCommentDlg �Ի���

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


// CEditCommentDlg ��Ϣ�������


void CGetStrDlg::OnBnClickedOk()
{
	UpdateData();

	if (!m_str.GetLength())
		return;
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	CDialogEx::OnOK();
}


BOOL CGetStrDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowText(m_Title);

	GetDlgItem(IDC_EDIT1)->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
}
