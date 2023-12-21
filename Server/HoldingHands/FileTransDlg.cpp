// MiniFileTransDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "FileTransDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "FileTransSrv.h"
#include "utils.h"
// CFileTransDlg 对话框

IMPLEMENT_DYNAMIC(CFileTransDlg, CDialogEx)

CFileTransDlg::CFileTransDlg(CFileTransSrv*pHandler, CWnd* pParent /*=NULL*/)
: CDialogEx(CFileTransDlg::IDD, pParent),
	m_pHandler(pHandler),
	m_ullTotalSize(0),
	m_ullFinishedSize(0),
	m_dwTotalCount(0),
	m_dwFinishedCount(0),
	m_dwFailedCount(0),
	m_ullCurrentFinishedSize(0),
	m_ullCurrentFileSize(0),
	m_TransferFinished(FALSE)
{
	m_pHandler->Get();
}

CFileTransDlg::~CFileTransDlg()
{
	m_pHandler->Put();
}

void CFileTransDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TRANS_PROGRESS, m_Progress);
	DDX_Control(pDX, IDC_RICHEDIT21, m_TransLog);
}


BEGIN_MESSAGE_MAP(CFileTransDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_MNFT_TRANS_INFO, OnTransInfo)
	ON_MESSAGE(WM_MNFT_FILE_TRANS_BEGIN, OnTransFileBegin)
	ON_MESSAGE(WM_MNFT_FILE_TRANS_FINISHED, OnTransFileFinished)
	ON_MESSAGE(WM_MNFT_FILE_DC_TRANSFERRED, OnTransFileDC)
	ON_MESSAGE(WM_MNFT_TRANS_FINISHED, OnTransFinished)
	ON_MESSAGE(WM_MNFT_ERROR,OnError)
END_MESSAGE_MAP()


// CFileTransDlg 消息处理程序

BOOL CFileTransDlg::OnInitDialog()
{
	char szIP[0x100];
	CDialogEx::OnInitDialog();
	// TODO:  在此添加额外的初始化
	//
	m_pHandler->GetPeerAddress(szIP);
	m_IP = szIP;

	//Set Progress..
	m_Progress.SetRange(0, 100);
	m_Progress.SetPos(0);

	//
	m_TransLog.LimitText(0x7fffffff);
	//
	m_pHandler->SetNotifyWindow(GetSafeHwnd());
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}

LRESULT CFileTransDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	TCHAR* szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}



void CFileTransDlg::PostNcDestroy()
{
	// TODO:  在此添加专用代码和/或调用基类
	delete this;
}


void CFileTransDlg::OnClose()
{
	m_pHandler->SetNotifyWindow();
	m_pHandler->Close();

	DestroyWindow();
}

CString CFileTransDlg::GetProgressString(ULONGLONG ullFinished, ULONGLONG ullTotal)
{
	LARGE_INTEGER liTotalSize, liFinishedSize;
	TCHAR szTotalSize[64], szFinishedSize[64];

	liTotalSize.QuadPart = ullTotal;
	liFinishedSize.QuadPart = ullFinished;

	GetStorageSizeString(liFinishedSize, szFinishedSize);
	GetStorageSizeString(liTotalSize, szTotalSize);

	//防止为0....
	if (liTotalSize.QuadPart == 0)
	{
		liTotalSize.QuadPart = 1;
	}
	CString Text;
	Text.Format(
		TEXT("%s / %s (%llu%%)"), 
		szFinishedSize, 
		szTotalSize,
		100 * liFinishedSize.QuadPart / liTotalSize.QuadPart);

	return Text;
}

//最开始的时候总的传输信息.
LRESULT CFileTransDlg::OnTransInfo(WPARAM wParam, LPARAM lParam)
{
	CFileTransSrv::MNFT_Trans_Info*pTransInfo = (CFileTransSrv::MNFT_Trans_Info*)wParam;
	m_ullTotalSize = pTransInfo->TotalSizeHi;
	m_ullTotalSize <<= 32;
	m_ullTotalSize |= pTransInfo->TotalSizeLo;

	m_dwTotalCount = pTransInfo->dwFileCount;
	//
	CString Text;
	if (lParam == MNFT_DUTY_RECEIVER)
		Text.Format(TEXT("[%s] Downloading..."), m_IP.GetBuffer());
	else
		Text.Format(TEXT("[%s] Uploading..."), m_IP.GetBuffer());

	SetWindowText(Text);

	//设置总的传输信息.
	Text = GetProgressString(m_ullFinishedSize, m_ullTotalSize);
	GetDlgItem(IDC_TOTAL_PROGRESS)->SetWindowText(Text);
	return 0;
}


//当一个文件开始的时候会调用这个.
LRESULT CFileTransDlg::OnTransFileBegin(WPARAM wParam, LPARAM lParam)
{
	CFileTransSrv::FileInfo*pFile = (CFileTransSrv::FileInfo*)wParam;
	CString Text;
	int length = m_TransLog.GetTextLength();
	Text.Format(TEXT("Processing: %s"), pFile->RelativeFilePath);
	m_TransLog.SetSel(length, length);

	m_TransLog.ReplaceSel(Text);

	//设置当前文件
	Text.Format(
		TEXT("%s (%d/%d)"), 
		pFile->RelativeFilePath,
		m_dwFinishedCount + 1,
		m_dwTotalCount);
	
	GetDlgItem(IDC_CURRENT_FILE)->SetWindowText(Text);

	//当前文件进度.
	m_ullCurrentFileSize = pFile->dwFileLengthHi;
	m_ullCurrentFileSize <<= 32;
	m_ullCurrentFileSize |= pFile->dwFileLengthLo;

	m_ullCurrentFinishedSize = 0;
	
	Text = GetProgressString(m_ullCurrentFinishedSize, m_ullCurrentFileSize);
	GetDlgItem(IDC_CURRENT_PROGRESS)->SetWindowText(Text);
	return 0;
}

//每一个文件传输结束后会调用这个.


#define FILE_TRANS_MAX_LOG_SIZE		(50 * 1024)

LRESULT CFileTransDlg::OnTransFileFinished(WPARAM wParam, LPARAM lParam)
{
	int length = m_TransLog.GetTextLength();
	CString Text = TEXT("    -Successed\r\n");

	if (wParam != MNFT_STATU_SUCCESS)
	{
		m_dwFailedCount++;
		Text = "    -Failed\r\n";
	}

	//记录结果.
	m_TransLog.SetSel(length, length);
	m_TransLog.ReplaceSel(Text);
	m_dwFinishedCount++;
	return 0;
}

//
LRESULT CFileTransDlg::OnTransFileDC(WPARAM wParam, LPARAM lParam)
{
	m_ullFinishedSize += wParam;
	m_ullCurrentFinishedSize += wParam;
	CString Text;

	//更新当前文件进度.
	Text = GetProgressString(m_ullCurrentFinishedSize, m_ullCurrentFileSize);
	GetDlgItem(IDC_CURRENT_PROGRESS)->SetWindowText(Text);

	//更新总进度
	Text = GetProgressString(m_ullFinishedSize, m_ullTotalSize);
	GetDlgItem(IDC_TOTAL_PROGRESS)->SetWindowText(Text);

	//更新进度条;
	m_Progress.SetPos(m_ullFinishedSize * 100ull / m_ullTotalSize);
	return 0;
}

//最终结束的时候会调用这个...
LRESULT CFileTransDlg::OnTransFinished(WPARAM wParam, LPARAM lParam)
{
	CString Text;
	Text.Format(
		TEXT("[%s] Transfer Complete!  Failed : %d/%d "),
		m_IP.GetBuffer(),
		m_dwFailedCount,
		m_dwTotalCount
	);

	SetWindowText(Text);

	m_TransferFinished = TRUE;
	return 0;
}



void CFileTransDlg::OnCancel()
{
}


void CFileTransDlg::OnOK()
{
}
