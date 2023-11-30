// MiniDownloadDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "FileDownloadDlg.h"
#include "afxdialogex.h"
#include "FileDownloadSrv.h"
#include "utils.h"

// CFileDownloadDlg 对话框

IMPLEMENT_DYNAMIC(CFileDownloadDlg, CDialog)

CFileDownloadDlg::CFileDownloadDlg(CFileDownloadSrv* pHandler, CWnd* pParent /*=NULL*/)
: CDialog(CFileDownloadDlg::IDD, pParent),
	m_pHandler(pHandler),
	m_DownloadFinished(FALSE)
{
	m_pHandler->Get();
}

CFileDownloadDlg::~CFileDownloadDlg()
{
	m_pHandler->Put();
}

void CFileDownloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DOWNLOAD_PROGRESS, m_Progress);
}


BEGIN_MESSAGE_MAP(CFileDownloadDlg, CDialog)
	ON_MESSAGE(WM_MNDD_FILEINFO, OnFileInfo)
	ON_MESSAGE(WM_MNDD_DOWNLOAD_RESULT, OnDownloadResult)
	ON_MESSAGE(WM_MNDD_ERROR,OnError)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CFileDownloadDlg 消息处理程序

LRESULT CFileDownloadDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	TCHAR * szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}


LRESULT CFileDownloadDlg::OnFileInfo(WPARAM wParam, LPARAM lParam)
{
	LPVOID * ArgList = (LPVOID*)lParam;
	CString FileName((TCHAR*)ArgList[0]);
	CString Url((TCHAR*)ArgList[1]);
	//
	int TotalSize = (int)ArgList[2];

	GetDlgItem(IDC_URL)->SetWindowText(Url);
	GetDlgItem(IDC_FILE_NAME)->SetWindowText(FileName);

	//if (TotalSize == -1){
	//	m_Progress.SetMarquee(TRUE, 30);//进度条设置为未定义.
	//}
	return 0;
}

LRESULT CFileDownloadDlg::OnDownloadResult(WPARAM wParam, LPARAM lParam)
{

	/*
	LPVOID ArgList[2];
	ArgList[0] = (LPVOID)root["total_size"].asInt();
	ArgList[1] = (LPVOID)root["finished_size"].asInt();
	*/
	int Argc = wParam;
	LPVOID * ArgList = (LPVOID*)lParam;
	int TotalSize = (int)ArgList[0];
	int FinishedSize = (int)ArgList[1];
	int Finished = (int)ArgList[2];
	long long Progress = 0;

	LARGE_INTEGER liFinished, liTotal;
	TCHAR strFinished[128], strTotal[128];
	
	liFinished.QuadPart = FinishedSize;
	liTotal.QuadPart = TotalSize;

	GetStorageSizeString(liFinished, strFinished);

	if (TotalSize != -1 && TotalSize)
	{	
		GetStorageSizeString(liTotal, strTotal);
		CString strText;
		strText.Format(TEXT("%s / %s"), strFinished, strTotal);
		GetDlgItem(IDC_PROGRESS)->SetWindowText(strText);

		Progress = FinishedSize;
		Progress *= 100;
		Progress /= TotalSize;
		m_Progress.SetPos(Progress);
	}
	else
	{
		CString strText;
		strText.Format(TEXT("%s / unknown"), strFinished);
		GetDlgItem(IDC_PROGRESS)->SetWindowText(strText);
	}

	if (Finished)
	{
		char szIP[0x100]; 
		CString Title;
		m_pHandler->GetPeerAddress(szIP);
		Title.Format(TEXT("[%s] Download Finished"), CString(szIP));
		SetWindowText(Title);
		//
		m_DownloadFinished = TRUE;
	}
	return 0;
}



void CFileDownloadDlg::PostNcDestroy()
{
	delete this;
}

void CFileDownloadDlg::OnClose()
{
	m_pHandler->SetNotifyWindow();
	m_pHandler->Close();

	DestroyWindow();
}


BOOL CFileDownloadDlg::OnInitDialog()
{
	char szIP[0x100];
	CDialog::OnInitDialog();

	m_pHandler->GetPeerAddress(szIP);
	// TODO:  在此添加额外的初始化
	CString Title;
	Title.Format(TEXT("[%s] Download...."), CString(szIP));
	SetWindowText(Title);
	//
	m_Progress.SetRange(0, 100);
	m_Progress.SetPos(0);
	
	//
	m_pHandler->SetNotifyWindow(GetSafeHwnd());
	
	return TRUE;  // return TRUE unless you set the focus to a control
}



void CFileDownloadDlg::OnOK()
{
}


void CFileDownloadDlg::OnCancel()
{
}
