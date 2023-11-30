// AudioDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "AudioDlg.h"
#include "afxdialogex.h"
#include "AudioSrv.h"

// CAudioDlg 对话框

IMPLEMENT_DYNAMIC(CAudioDlg, CDialog)

CAudioDlg::CAudioDlg(CAudioSrv*pHandler, CWnd* pParent /*=NULL*/)
: CDialog(CAudioDlg::IDD, pParent),
m_pHandler(pHandler)
{
	m_pHandler->Get();
}

CAudioDlg::~CAudioDlg()
{
	m_pHandler->Put();
}

void CAudioDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK1, m_ListenLocal);
}


BEGIN_MESSAGE_MAP(CAudioDlg, CDialog)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_AUDIO_ERROR,OnError)
	ON_BN_CLICKED(IDC_CHECK1, &CAudioDlg::OnBnClickedCheck1)
END_MESSAGE_MAP()


// CAudioDlg 消息处理程序


void CAudioDlg::PostNcDestroy()
{
	delete this;	
}


void CAudioDlg::OnClose()
{
	m_pHandler->SetNotifyWindow(NULL);
	m_pHandler->Close();
	DestroyWindow();
}

void CAudioDlg::OnOK()
{

}


void CAudioDlg::OnCancel()
{

}


BOOL CAudioDlg::OnInitDialog()
{
	char szIP[0x100];
	CDialog::OnInitDialog();

	//set window text
	m_pHandler->GetPeerAddress(szIP);

	CString Title;
	Title.Format(TEXT("[%s] Microphone"), CString(szIP));
	SetWindowText(Title);

	//set notify window...
	m_pHandler->SetNotifyWindow(GetSafeHwnd());
	return TRUE;  // return TRUE unless you set the focus to a control
}

LRESULT CAudioDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	TCHAR *szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"),MB_OK|MB_ICONINFORMATION);
	return 0;
}


void CAudioDlg::OnBnClickedCheck1()
{
	if (m_ListenLocal.GetCheck())
	{
		if (IDYES != MessageBox(
			TEXT("Are you sure to send local voice to remote machine ?"),
			TEXT("Tips"),
			MB_YESNO))
		{
			m_ListenLocal.SetCheck(0);
			return;
		}
		m_pHandler->StartSendLocalVoice();
	}
	else
	{
		m_pHandler->StopSendLocalVoice();	
	}
}
