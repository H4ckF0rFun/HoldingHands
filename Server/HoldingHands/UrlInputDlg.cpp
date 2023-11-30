// UrlInputDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "UrlInputDlg.h"
#include "afxdialogex.h"


// CUrlInputDlg �Ի���

IMPLEMENT_DYNAMIC(CUrlInputDlg, CDialogEx)

CUrlInputDlg::CUrlInputDlg(CString Title , CWnd* pParent /*=NULL*/)
	: CDialogEx(CUrlInputDlg::IDD, pParent)
	, m_Url(_T(""))
	, m_Title(Title)
{

}

CUrlInputDlg::~CUrlInputDlg()
{
}

void CUrlInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Url);
	DDV_MaxChars(pDX, m_Url, 4095);
}


BEGIN_MESSAGE_MAP(CUrlInputDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CUrlInputDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CUrlInputDlg ��Ϣ�������


void CUrlInputDlg::OnBnClickedOk()
{
	UpdateData();
	if (!m_Url.GetLength())
		return;
	CDialogEx::OnOK();
}


BOOL CUrlInputDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	GetDlgItem(IDC_EDIT1)->SetFocus();

	if (m_Title.GetLength())
		SetWindowText(m_Title);

	return FALSE;  // return TRUE unless you set the focus to a control
}
