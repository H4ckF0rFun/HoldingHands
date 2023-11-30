// ChatInputName.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "ChatInputName.h"
#include "afxdialogex.h"


// CChatInputName �Ի���

IMPLEMENT_DYNAMIC(CChatInputName, CDialogEx)

CChatInputName::CChatInputName(CWnd* pParent /*=NULL*/)
	: CDialogEx(CChatInputName::IDD, pParent)
	, m_NickName(_T("Hacker"))
{

}

CChatInputName::~CChatInputName()
{
}

void CChatInputName::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_NickName);
}


BEGIN_MESSAGE_MAP(CChatInputName, CDialogEx)
	ON_BN_CLICKED(IDOK, &CChatInputName::OnBnClickedOk)
END_MESSAGE_MAP()


// CChatInputName ��Ϣ�������


void CChatInputName::OnBnClickedOk()
{
	UpdateData();
	if (m_NickName.GetLength() == 0)
		return;
	CDialogEx::OnOK();
}
