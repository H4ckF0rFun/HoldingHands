// ChatDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "ChatDlg.h"
#include "afxdialogex.h"
#include "ChatSrv.h"
#include "ChatInputName.h"
// CChatDlg 对话框

IMPLEMENT_DYNAMIC(CChatDlg, CDialogEx)

CChatDlg::CChatDlg(CChatSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CChatDlg::IDD, pParent)
	, m_Msg(_T("")),
	m_pHandler(pHandler)
{
	m_pHandler->Get();
}

CChatDlg::~CChatDlg()
{
	m_pHandler->Put();
}

void CChatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT2, m_Msg);
	DDX_Control(pDX, IDC_EDIT1, m_MsgList);
}


BEGIN_MESSAGE_MAP(CChatDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_CHAT_ERROR,OnError)
	ON_MESSAGE(WM_CHAT_BEGIN,OnChatBegin)
	ON_MESSAGE(WM_CHAT_GET_NICKNAME, OnGetNickName)
	ON_MESSAGE(WM_CHAT_MSG,OnChatMessage)
	ON_BN_CLICKED(IDOK, &CChatDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CChatDlg::OnInitDialog()
{
	char szIP[0x100];
	CDialogEx::OnInitDialog();
	
	//Set Window Text...
	CString Title;

	m_pHandler->GetPeerAddress(szIP);
	Title.Format(TEXT("[%s] Chat "), CString(szIP));
	SetWindowText(Title);

	//disable some windows..
	GetDlgItem(IDOK)->EnableWindow(FALSE);

	//
	m_pHandler->SetNotifyWindow(GetSafeHwnd());

	ShowWindow(SW_HIDE);
	return TRUE; 
}



// CChatDlg 消息处理程序

void CChatDlg::PostNcDestroy()
{
	delete this;
}

void CChatDlg::OnClose()
{
	m_pHandler->SetNotifyWindow();
	m_pHandler->Close();

	DestroyWindow();
}



LRESULT CChatDlg::OnError(WPARAM wParam, LPARAM lParma){
	TCHAR*szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}

LRESULT CChatDlg::OnChatBegin(WPARAM wParam, LPARAM lParam)
{
	GetDlgItem(IDOK)->EnableWindow(TRUE);
	return 0;
}
LRESULT CChatDlg::OnChatMessage(WPARAM wParam, LPARAM lParam)
{
	char * szMessage = (char*)wParam;
	//显示到屏幕上
	CString NewMsg;
	NewMsg.Format(TEXT("[Victim]: %s\r\n"), szMessage);
	m_MsgList.SetSel(-1);
	m_MsgList.ReplaceSel(NewMsg);

	return 0;
}

LRESULT CChatDlg::OnGetNickName(WPARAM wParam, LPARAM lParam)
{
	TCHAR* szNickName = (TCHAR*)wParam;
	CChatInputName input(this);
	if (input.DoModal() == IDOK)
	{
		lstrcpy(szNickName, input.m_NickName.GetBuffer());
	}
	else
	{
		lstrcpy(szNickName, TEXT("Hacker"));
	}

	ShowWindow(SW_SHOW);
	return 0;
}

void CChatDlg::OnBnClickedOk()
{
	UpdateData();
	if (m_Msg.GetLength() == 0)
		return;

	m_pHandler->SendText(m_Msg.GetBuffer());

	CString MyMsg;
	MyMsg.Format(TEXT("[me]:%s\r\n"), m_Msg.GetBuffer());

	m_MsgList.SetSel(-1);
	m_MsgList.ReplaceSel(MyMsg);
	
	m_Msg = "";

	UpdateData(FALSE);
}


void CChatDlg::OnOK()
{
}


void CChatDlg::OnCancel()
{
}
