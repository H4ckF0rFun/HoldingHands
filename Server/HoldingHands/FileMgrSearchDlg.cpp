// FileMgrSearchDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "FileMgrSearchDlg.h"
#include "afxdialogex.h"
#include "FileManagerDlg.h"
#include "FileMgrSearchSrv.h"
#include "utils.h"
// CFileMgrSearchDlg 对话框

IMPLEMENT_DYNAMIC(CFileMgrSearchDlg, CDialogEx)

CFileMgrSearchDlg::CFileMgrSearchDlg(CFileMgrSearchSrv*pHandler,CWnd* pParent /*=NULL*/)
	: CDialogEx(CFileMgrSearchDlg::IDD, pParent)
	, m_TargetName(_T(""))
	, m_StartLocation(_T(""))
	, m_pHandler(pHandler)
{
	m_pHandler->Get();
}

CFileMgrSearchDlg::~CFileMgrSearchDlg()
{
	m_pHandler->Put();
}

void CFileMgrSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_TargetName);
	DDX_Text(pDX, IDC_EDIT3, m_StartLocation);
	DDX_Control(pDX, IDC_LIST1, m_ResultList);
}


BEGIN_MESSAGE_MAP(CFileMgrSearchDlg, CDialogEx)
	ON_MESSAGE(WM_FILE_MGR_SEARCH_FOUND,OnFound)
	ON_MESSAGE(WM_FILE_MGR_SEARCH_OVER, OnOver)
	ON_MESSAGE(WM_FILE_MGR_SEARCH_ERROR, OnError)
	ON_BN_CLICKED(IDOK, &CFileMgrSearchDlg::OnBnClickedOk)
	ON_WM_CLOSE()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CFileMgrSearchDlg::OnNMDblclkList1)
END_MESSAGE_MAP()


// CFileMgrSearchDlg 消息处理程序


BOOL CFileMgrSearchDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CString Title;
	char szIP[0x100];

	// TODO:  在此添加额外的初始化
	m_ResultList.InsertColumn(0, TEXT("Name"), LVCFMT_LEFT, 130);
	m_ResultList.InsertColumn(1, TEXT("Location"), LVCFMT_LEFT, 370);
	//
	m_ResultList.SetImageList(CFileManagerDlg::pLargeImageList, LVSIL_NORMAL);
	m_ResultList.SetImageList(CFileManagerDlg::pSmallImageList, LVSIL_SMALL);
	//

	m_ResultList.SetExtendedStyle((~ LVS_EX_CHECKBOXES)&
		(m_ResultList.GetExStyle() | LVS_EX_FULLROWSELECT));

	//
	m_pHandler->GetPeerAddress(szIP);
	CString ip(szIP);

	Title.Format(TEXT("[%s] Search"), ip.GetBuffer());
	SetWindowText(Title);
	//
	m_pHandler->SetNotifyWindow(GetSafeHwnd());
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}



LRESULT CFileMgrSearchDlg::OnFound(WPARAM wParam, LPARAM lParam)
{
	struct FindFile
	{
		UINT32 dwFileAttribute;
		TCHAR szFileName[2];
	};

	int idx = 0;
	SHFILEINFO si = { 0 };
	FindFile*pFindFile = (FindFile*)wParam;
	TCHAR *szLocation = pFindFile->szFileName;
	TCHAR *szFileName = StrStr(pFindFile->szFileName, TEXT("\n"));

	if (szFileName == NULL)
		return 0;

	*szFileName++ = NULL;

	SHGetFileInfo(
		szFileName, 
		pFindFile->dwFileAttribute, &
		si, 
		sizeof(si),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX
	);


	idx = m_ResultList.GetItemCount();
	m_ResultList.InsertItem(idx, szFileName, si.iIcon);
	m_ResultList.SetItemText(idx, 1, szLocation);
	return 0;
}


LRESULT CFileMgrSearchDlg::OnOver(WPARAM wParam, LPARAM lParam)
{
	char szIP[0x100] = { 0 };
	CString Title;
	CWnd*pButton = GetDlgItem(IDOK);
	
	pButton->SetWindowText(TEXT("Search"));
	m_pHandler->GetPeerAddress(szIP);
	
	Title.Format(TEXT("[%s] Found: %d "), szIP, m_ResultList.GetItemCount());
	SetWindowText(Title);
	return 0;
}

void CFileMgrSearchDlg::OnBnClickedOk()
{
	CWnd*pButton = GetDlgItem(IDOK);
	CString Text;
	char szIP[0x100];

	pButton->GetWindowText(Text);

	if (Text == "Stop"){
		m_pHandler->Stop();
	}
	else 
	{
		//Search
		UpdateData();
		if (m_TargetName.GetLength() == 0 || m_StartLocation.GetLength() == 0)
			return;

		m_pHandler->GetPeerAddress(szIP);
		m_pHandler->Search((m_StartLocation + TEXT("\n") + m_TargetName).GetBuffer());

		//开始查找
		Text.Format(TEXT("[%s] Searching..."), szIP);
		SetWindowText(Text);

		pButton->SetWindowText(TEXT("Stop"));
		m_ResultList.DeleteAllItems();
		
	}
}



void CFileMgrSearchDlg::PostNcDestroy()
{
	// TODO:  在此添加专用代码和/或调用基类
	delete this;
}

void CFileMgrSearchDlg::OnClose()
{
	m_pHandler->SetNotifyWindow();
	m_pHandler->Close();

	DestroyWindow();
}



LRESULT CFileMgrSearchDlg::OnError(WPARAM wParam, LPARAM lParam){
	TCHAR *szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}

//双击拷贝到剪切板

void CFileMgrSearchDlg::OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;
	//
	CString FileName;
	if (!m_ResultList.GetSelectedCount())
		return;

	POSITION pos = m_ResultList.GetFirstSelectedItemPosition();
	int nItem = m_ResultList.GetNextSelectedItem(pos);

	FileName += m_ResultList.GetItemText(nItem, 1);
	FileName += TEXT("\\");
	FileName += m_ResultList.GetItemText(nItem, 0);

	HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, sizeof(TCHAR) * (FileName.GetLength()+ 1));
	TCHAR* Buffer = (TCHAR*)GlobalLock(hGlobal);
	lstrcpy(Buffer, FileName);

	GlobalUnlock(hGlobal);
	OpenClipboard();
	EmptyClipboard();
#ifdef UNICODE 
	SetClipboardData(CF_UNICODETEXT, hGlobal);
#else
	SetClipboardData(CF_TEXT, hGlobal);
#endif
	CloseClipboard();
}


void CFileMgrSearchDlg::OnOK()
{
}


void CFileMgrSearchDlg::OnCancel()
{
}


