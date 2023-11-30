// ChatInputName.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "ChatInputName.h"
#include "afxdialogex.h"


// CChatInputName 对话框

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


// CChatInputName 消息处理程序


void CChatInputName::OnBnClickedOk()
{
	UpdateData();
	if (m_NickName.GetLength() == 0)
		return;
	CDialogEx::OnOK();
}
