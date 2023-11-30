// FileManagerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "FileManagerDlg.h"
#include "afxdialogex.h"
#include "FileManagerSrv.h"
#include "FileSelectDlg.h"
#include "UrlInputDlg.h"
#include "utils.h"
#include "dbg.h"

//IID_IImageList
#include <commoncontrols.h>
// CFileManagerDlg 对话框

IMPLEMENT_DYNAMIC(CFileManagerDlg, CDialogEx)

CFileManagerDlg::CFileManagerDlg(CFileManagerSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CFileManagerDlg::IDD, pParent)
	, m_Location(_T("")),
	m_pHandler(pHandler),
	m_DestroyAfterDisconnect(FALSE)
{
	m_order = -1;
	m_LastSortColum = -1;
	m_pHandler->Get();
}

CFileManagerDlg::~CFileManagerDlg()
{
	m_pHandler->Put();
}

void CFileManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_FileList);
	DDX_Text(pDX, IDC_EDIT1, m_Location);
}


BEGIN_MESSAGE_MAP(CFileManagerDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, OnToolbarDropDown)
	ON_COMMAND(ID_VIEW_ICON, &CFileManagerDlg::OnViewIcon)
	ON_COMMAND(ID_VIEW_DETAILEDINFORMATION, &CFileManagerDlg::OnViewDetailedinformation)
	ON_COMMAND(ID_VIEW_LIST, &CFileManagerDlg::OnViewList)
	ON_COMMAND(ID_FM_UP,OnUp)
	ON_COMMAND(ID_FM_REFRESH,OnRefresh)
	ON_COMMAND(ID_FM_GO,OnGo)
	ON_COMMAND(ID_FM_SEARCH,OnSearch)
	ON_COMMAND(ID_FM_NEWFOLDER, OnMenuNewfolder)
	ON_COMMAND(ID_FM_DEL,OnMenuDelete)
	ON_MESSAGE(WM_FMDLG_CHDIR,OnChDir)
	ON_MESSAGE(WM_FMDLG_LIST,OnFillList)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CFileManagerDlg::OnNMDblclkList1)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CFileManagerDlg::OnNMRClickList1)
	ON_COMMAND(ID_UPLOAD_FROMDISK, &CFileManagerDlg::OnUploadFromdisk)
	ON_COMMAND(ID_UPLOAD_FROMURL, &CFileManagerDlg::OnUploadFromurl)
	ON_COMMAND(ID_MENU_DOWNLOAD, &CFileManagerDlg::OnMenuDownload)

	ON_MESSAGE(WM_FMDLG_NEWFOLDER_SUCCESS, OnNewfolderSuccess)
	ON_COMMAND(ID_RUNFILE_HIDE, &CFileManagerDlg::OnRunfileHide)
	ON_COMMAND(ID_RUNFILE_NORMAL, &CFileManagerDlg::OnRunfileNormal)
	ON_COMMAND(ID_MENU_RENAME, &CFileManagerDlg::OnMenuRename)
	ON_COMMAND(ID_MENU_NEWFOLDER, &CFileManagerDlg::OnMenuNewfolder)
	ON_COMMAND(ID_MENU_DELETE, &CFileManagerDlg::OnMenuDelete)
	ON_COMMAND(ID_MENU_COPY, &CFileManagerDlg::OnMenuCopy)
	ON_COMMAND(ID_MENU_CUT, &CFileManagerDlg::OnMenuCut)
	ON_COMMAND(ID_MENU_PASTE, &CFileManagerDlg::OnMenuPaste)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST1, &CFileManagerDlg::OnLvnEndlabeleditList1)
	ON_MESSAGE(WM_FMDLG_ERROR,OnError)
	ON_WM_NCDESTROY()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, &CFileManagerDlg::OnLvnColumnclickList1)
END_MESSAGE_MAP()


// CFileManagerDlg 消息处理程序

HIMAGELIST CFileManagerDlg::hImageList_SmallIcon = NULL;
HIMAGELIST CFileManagerDlg::hImageList_LargeIcon = NULL; 
CImageList* CFileManagerDlg::pLargeImageList = NULL;
CImageList* CFileManagerDlg::pSmallImageList = NULL;


BOOL CFileManagerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	char szIP[0x100];
	// TODO:  在此添加额外的初始化
	if (!m_ToolBar.CreateEx(
		this, 
		TBSTYLE_FLAT | TBSTYLE_LIST,
		WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS
		) ||
		!m_ToolBar.LoadToolBar(IDR_FM_TOOLBAR))
	{
		TRACE0("Failed to Create Dialog ToolBar\n");
		EndDialog(IDCANCEL);
	}

	m_ToolBar.ShowWindow(SW_SHOW);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, 0, 0);

	VERIFY(m_ToolBar.SetButtonText(0, TEXT("Up")));
	VERIFY(m_ToolBar.SetButtonText(1, TEXT("Delete")));
	VERIFY(m_ToolBar.SetButtonText(2, TEXT("Search")));
	VERIFY(m_ToolBar.SetButtonText(3, TEXT("Refresh")));
	VERIFY(m_ToolBar.SetButtonText(4, TEXT("Folder")));
	VERIFY(m_ToolBar.SetButtonText(5, TEXT("View")));
	VERIFY(m_ToolBar.SetButtonText(6, TEXT("Go")));
	
	//为画图工具栏的按钮添加下拉菜单
	m_ToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
	//取值TBSTYLE_EX_DRAWDDARROWS，可以为某一个按钮添加下拉按钮。
	DWORD dwStyle = m_ToolBar.GetButtonStyle(5);
	dwStyle |= TBSTYLE_DROPDOWN; 
	m_ToolBar.SetButtonStyle(5, dwStyle);

	HINSTANCE hInstance = GetModuleHandle(NULL);
	
	m_ImageList.Create(24, 24, ILC_COLOR32 | ILC_MASK, 0, 0);

	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_UP)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_DEL)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_SEARCH)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_REFRESH)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_NEWFOLDER)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_VIEW)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_GO)));

	m_ToolBar.GetToolBarCtrl().SetImageList(&m_ImageList);

	if (hImageList_SmallIcon == NULL){
		SHGetImageList(SHIL_SMALL, IID_IImageList, (void**)&hImageList_SmallIcon);
	}

	if (hImageList_LargeIcon == NULL){
		SHGetImageList(SHIL_LARGE, IID_IImageList, (void**)&hImageList_LargeIcon);
	}

	if (pLargeImageList == 0){
		pLargeImageList = new CImageList;
		pLargeImageList->Attach(hImageList_LargeIcon);
	}

	if (pSmallImageList == 0){
		pSmallImageList = new CImageList;
		pSmallImageList->Attach(hImageList_SmallIcon);
	}
	//调整位置
	Resize();

	//绑定图标列表
	m_FileList.SetImageList(pLargeImageList, LVSIL_NORMAL);
	m_FileList.SetImageList(pSmallImageList, LVSIL_SMALL);
	m_FileList.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);

	//Set Title.

	m_pHandler->GetPeerAddress(szIP);
	CString Title;
	CString ip(szIP);

	Title.Format(TEXT("[%s] FileManager"), ip.GetBuffer());
	SetWindowText(Title);
	
	//禁用窗口,请求目录
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);
	
	//
	dbg_log("GetSafeHwnd(): %08x\n", GetSafeHwnd());
	m_pHandler->SetNotifyWindow(GetSafeHwnd());
	
	//root directory..
	m_pHandler->ChDir(m_Location.GetBuffer());
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}


void CFileManagerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (m_ToolBar.m_hWnd == NULL)
		return;
	// TODO:  在此处添加消息处理程序代码
	Resize();
}

void CFileManagerDlg::Resize()
{
	RECT ClientRect;
	GetClientRect(&ClientRect);
	ClientRect.top += 67;
	m_FileList.MoveWindow(&ClientRect);

	ClientRect.top = 37;
	ClientRect.bottom = ClientRect.top + 27;
	ClientRect.left = 65;
	GetDlgItem(IDC_EDIT1)->MoveWindow(&ClientRect);

	ClientRect.top += 3;
	ClientRect.right = 65;
	ClientRect.left = 5;
	GetDlgItem(IDC_STATIC)->MoveWindow(&ClientRect);
}

/*
2 种清空
	
	使 Dlg 一定比 pHandler迟销毁...

*/
void CFileManagerDlg::PostNcDestroy()
{
	delete this;
}


void CFileManagerDlg::OnClose()
{
	m_pHandler->SetNotifyWindow();
	m_pHandler->Close();
	DestroyWindow();
}

LRESULT CFileManagerDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	TCHAR * szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}


void CFileManagerDlg::OnToolbarDropDown(NMHDR *pnmhdr, LRESULT *plr)
{
	LPNMTOOLBAR pnmtb = reinterpret_cast<LPNMTOOLBAR>(pnmhdr);
	if (pnmtb->iItem != ID_FM_VIEW)
		return;

	CMenu menu;
	menu.LoadMenu(IDR_FM_MENU);

	CRect rect;
	m_ToolBar.GetItemRect(5,&rect);
	m_ToolBar.ClientToScreen(&rect);
	menu.GetSubMenu(1)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, 
		rect.left, rect.bottom, this, &rect);
}

void CFileManagerDlg::OnViewIcon()
{
	// TODO:  在此添加命令处理程序代码
	m_FileList.ModifyStyle(LVS_TYPEMASK, LVS_ICON);
}


void CFileManagerDlg::OnViewDetailedinformation()
{
	// TODO:  在此添加命令处理程序代码
	m_FileList.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
}


void CFileManagerDlg::OnViewList()
{
	// TODO:  在此添加命令处理程序代码
	m_FileList.ModifyStyle(LVS_TYPEMASK, LVS_LIST);
}

void CFileManagerDlg::OnUp()
{
	
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);
	//上一层目录.s
	m_pHandler->Up();
	
}

LRESULT CFileManagerDlg::OnChDir(WPARAM Statu, LPARAM CurLocation)
{
	if (!Statu)
	{
		MessageBox(TEXT("Access Failed!"));
		m_FileList.EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
	}

	//成功的话等待对方把目录发来,刷新location
	m_Location = (TCHAR*)CurLocation;
	UpdateData(FALSE);
	return 0;
}



CString GetSize(DWORD lo,DWORD hi)
{
	TCHAR  strBuffer[128];
	LARGE_INTEGER liSize;
	liSize.LowPart = lo, liSize.HighPart = hi;

	GetStorageSizeString(liSize, strBuffer);
	return strBuffer;
}

void CFileManagerDlg::FillDriverList(DriverInfo*pDis, int count)
{
	m_FileList.DeleteAllItems();
	//删除所有列
	while (m_FileList.DeleteColumn(0));
	//
	m_FileList.InsertColumn(0, TEXT("Name"), LVCFMT_LEFT, 250);
	m_FileList.InsertColumn(1, TEXT("Type"), LVCFMT_LEFT, 180);
	m_FileList.InsertColumn(2, TEXT("FileSystem"), LVCFMT_LEFT, 180);
	m_FileList.InsertColumn(3, TEXT("Total"), LVCFMT_LEFT, 180);
	m_FileList.InsertColumn(4, TEXT("Free"), LVCFMT_LEFT, 180);

	int nIconIdx = 0;
	for (int i = 0; i < count; i++)
	{
		//查找图标索引
		if (pDis[i].szName[0] == 'A' || pDis[i].szName[0] == 'B')
		{
			nIconIdx = 6;
		}
		else
		{
			switch (pDis->dwType)
			{
			case DRIVE_REMOVABLE:
				nIconIdx = 7;
				break;
			case DRIVE_FIXED:
				nIconIdx = 8;
				break;
			case DRIVE_REMOTE:
				nIconIdx = 9;
				break;
			case DRIVE_CDROM:
				nIconIdx = 11;
				break;
			default:
				nIconIdx = 8;
				break;
			}
		}
		m_FileList.InsertItem(i, &pDis[i].szName[1], nIconIdx);
		m_FileList.SetItemText(i, 1, pDis[i].szTypeName);
		m_FileList.SetItemText(i, 2, pDis[i].szFileSystem);
		m_FileList.SetItemText(i, 3, GetSize(pDis[i].Total.LowPart, pDis[i].Total.HighPart));
		m_FileList.SetItemText(i, 4, GetSize(pDis[i].Free.LowPart, pDis[i].Free.HighPart));
		//保存盘符.
		m_FileList.SetItemData(i, pDis[i].szName[0]);
	}
}


void CFileManagerDlg::AddItem(int idx,FMFileInfo * pis){

	SHFILEINFO si = { 0 };
	FILETIME ft = { 0 };
	CString Size;
	//添加到list里面
	SHGetFileInfo(
		pis->szFileName, 
		pis->dwFileAttribute, 
		&si, 
		sizeof(si),
		SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME | SHGFI_SYSICONINDEX
	);

	m_FileList.InsertItem(idx, pis->szFileName, si.iIcon);

	m_FileList.SetItemText(idx, 1, si.szTypeName);

	if (!(pis->dwFileAttribute&FILE_ATTRIBUTE_DIRECTORY))
		m_FileList.SetItemText(idx, 2, GetSize(pis->dwFileSizeLo, pis->dwFileSizeHi));

	
	memcpy(&ft, &pis->dwLastWriteLo, sizeof(FILETIME));

	CTime time(ft);
	m_FileList.SetItemText(idx, 3, time.Format("%Y-%m-%d %H:%M"));
	//保存文件属性.
	m_FileList.SetItemData(idx, pis->dwFileAttribute);
}

void CFileManagerDlg::FillFileList(FMFileInfo*pis)
{
	m_FileList.DeleteAllItems();
	//删除所有列
	while (m_FileList.DeleteColumn(0));

	m_FileList.InsertColumn(0, TEXT("Name"), LVCFMT_LEFT,360);
	m_FileList.InsertColumn(1, TEXT("Type"), LVCFMT_LEFT, 160);
	m_FileList.InsertColumn(2, TEXT("Size"), LVCFMT_LEFT, 160);
	m_FileList.InsertColumn(3, TEXT("LastWrite"), LVCFMT_LEFT, 160);

	FMFileInfo End = { 0 };
	int idx = 0;
	m_FileList.SetRedraw(FALSE);
	while (TRUE)
	{
		//结束
		if (!memcmp(&End, pis, sizeof(FMFileInfo)))
			break;

		AddItem(idx, pis);
		idx++;

		pis = (FMFileInfo*)(((char*)pis) +
			((char*)pis->szFileName - (char*)pis) + 
			sizeof(TCHAR)*(lstrlen(pis->szFileName) + 1));
	}
	m_FileList.SetRedraw(TRUE);
}

LRESULT CFileManagerDlg::OnFillList(WPARAM wParam, LPARAM lParam)
{
	char*pBuff = (char*)lParam;
	DWORD dwRead = wParam;

	if (m_Location.GetLength())
		FillFileList((FMFileInfo*)pBuff);
	else
		FillDriverList((DriverInfo*)pBuff,dwRead/sizeof(DriverInfo));
	
	//启用窗口.
	m_FileList.EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
	return 0;
}

void CFileManagerDlg::OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;
	//没有选中项直接返回.

	if (m_FileList.GetSelectedCount() == 0)
		return;
	//点击的是文件，不管它.
	if (m_Location.GetLength() && !(m_FileList.GetItemData(pNMItemActivate->iItem) & FILE_ATTRIBUTE_DIRECTORY))
		return;

	if (m_pHandler == nullptr){
		return;
	}

	//禁用窗口,请求目录
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);

	if (m_Location.GetLength())
	{
		CString NewDir = m_Location;
		NewDir += "\\";
		NewDir += m_FileList.GetItemText(pNMItemActivate->iItem, 0);

		m_pHandler->ChDir(NewDir.GetBuffer());
	}
	else
	{
		//驱动器.
		TCHAR szRoot[4] = { 0 };
		szRoot[0] = m_FileList.GetItemData(pNMItemActivate->iItem);
		szRoot[1] = ':';
		szRoot[2] = '\\';
		
		m_pHandler->ChDir(szRoot);
	}
}


void CFileManagerDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;
	CMenu menu;
	menu.LoadMenu(IDR_FM_MENU);
	POINT pt;
	GetCursorPos(&pt);
	//FileList 右键菜单
	menu.GetSubMenu(0)->TrackPopupMenu(
		TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
		pt.x,
		pt.y, 
		this);
}

/*
	任何模态对话框的创建应该在有IO消息到达时创建,因为那时候窗口对象不会被delete掉.
	不要在窗口点击事件里面创建模态对话框.
*/

void CFileManagerDlg::OnUploadFromdisk(){
	if (!m_Location.GetLength())
		return ;

	CFileSelectDlg dlg(this);
	if (IDOK != dlg.DoModal() ||
		dlg.m_FileList.GetLength() == 0)
	{
		return ;
	}

	//对方开启MINIFileTrans模块.
	m_pHandler->UploadFromDisk(
		dlg.m_CurrentDirectory,
		dlg.m_FileList
	);

	return ;
}

void CFileManagerDlg::OnUploadFromurl(){
	if (!m_Location.GetLength())
		return;

	CUrlInputDlg dlg(TEXT("Please input url:(HTTP(s)/FTP)"),this);

	if (IDOK != dlg.DoModal() ||
		dlg.m_Url.GetLength() == 0)
	{
		return;
	}
	//对方开启MINIDownload模块.
	m_pHandler->UploadFromUrl(dlg.m_Url.GetBuffer());
}

void CFileManagerDlg::OnMenuDownload(){
	TCHAR szPathName[MAX_PATH] = { 0 };
	BROWSEINFO bInfo = { 0 };
	LPITEMIDLIST lpDlist;
	POSITION pos;
	CString FileList;

	if (!m_Location.GetLength() || 
		m_FileList.GetSelectedCount() == 0)
		return ;
	//
	
	bInfo.hwndOwner = m_hWnd; // 父窗口;
	bInfo.lpszTitle = TEXT("Please Select Save Path");
	bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_UAHINT;
	
	lpDlist = SHBrowseForFolder(&bInfo);
	
	if (NULL == lpDlist)
	{ // 单击了确定按钮;
		return;
	}

	SHGetPathFromIDList(lpDlist, szPathName);
	pos = m_FileList.GetFirstSelectedItemPosition();

	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		FileList += m_FileList.GetItemText(idx, 0);
		FileList += L"\n";
	}

	m_pHandler->Download(szPathName, FileList);
}

void CFileManagerDlg::OnMenuRename(){
	if (m_FileList.GetSelectedCount() == 0)
		return ;
	//只重命名一个.
	POSITION pos = m_FileList.GetFirstSelectedItemPosition();
	
	if (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		m_oldFileName = m_FileList.GetItemText(idx, 0);	
		m_FileList.EditLabel(idx);
	}
}

void CFileManagerDlg::OnMenuNewfolder(){
	m_FileList.SetFocus();
	m_pHandler->NewFolder();
}


void CFileManagerDlg::OnRunfileHide()
{
	if (m_FileList.GetSelectedCount() == 0)
		return;
	CString Files;
	POSITION pos = m_FileList.GetFirstSelectedItemPosition();

	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		Files += m_FileList.GetItemText(idx, 0);
		Files += L"\n";
	}

	m_pHandler->RunFileHide(Files.GetBuffer());

}


void CFileManagerDlg::OnRunfileNormal()
{
	if (m_FileList.GetSelectedCount() == 0)
		return;
	CString Files;
	POSITION pos = m_FileList.GetFirstSelectedItemPosition();

	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		Files += m_FileList.GetItemText(idx, 0);
		Files += L"\n";
	}

	m_pHandler->RunFileNormal(Files.GetBuffer());
}


void CFileManagerDlg::OnGo()
{
	UpdateData();
	//禁用窗口,请求目录
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);
	//修改目录就行
	m_pHandler->ChDir(m_Location.GetBuffer());
}

void CFileManagerDlg::OnRefresh()
{
	//禁用窗口,请求目录
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);
	//修改目录就行
	m_pHandler->Refresh();
}


LRESULT	CFileManagerDlg::OnNewfolderSuccess(WPARAM wParam, LPARAM lParam)
{
	FMFileInfo*pis = (FMFileInfo*)wParam;
	int idx = m_FileList.GetItemCount();

	//添加项目.
	AddItem(idx, pis);
	m_oldFileName = pis->szFileName;
	m_FileList.EditLabel(idx);
	return 0;
}


void CFileManagerDlg::OnOK()
{
	//屏蔽回车键.
	OnGo();
}


void CFileManagerDlg::OnMenuDelete()
{
	if (m_FileList.GetSelectedCount() == 0)
		return;

	CString s;
	POSITION pos = m_FileList.GetFirstSelectedItemPosition();

	while (pos){
		int idx = m_FileList.GetNextSelectedItem(pos);
		s += m_FileList.GetItemText(idx, 0);
		s += L"\n";
	}
	
	m_pHandler->Delete(s.GetBuffer());

	//刷新一下
	OnRefresh();
}


void CFileManagerDlg::OnMenuCopy()
{
	//驱动器目录不允许复制和cut
	if (m_FileList.GetSelectedCount() == 0 || m_Location.GetLength() == 0)
		return;
	CString s = m_Location + L"\n";

	POSITION pos = m_FileList.GetFirstSelectedItemPosition();
	while (pos){
		int idx = m_FileList.GetNextSelectedItem(pos);
		s += m_FileList.GetItemText(idx, 0);
		s += "\n";
	}
	
	m_pHandler->Copy(s.GetBuffer());
	
}


void CFileManagerDlg::OnMenuCut()
{
	//驱动器目录不允许复制和cut
	if (m_FileList.GetSelectedCount() == 0 || m_Location.GetLength() == 0)
		return;

	CString s = m_Location + L"\n";

	POSITION pos = m_FileList.GetFirstSelectedItemPosition();
	while (pos)	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		s += m_FileList.GetItemText(idx, 0);
		s += "\n";
	}
	
	m_pHandler->Cut(s.GetBuffer());
	
}


void CFileManagerDlg::OnMenuPaste()
{
	// TODO:  在此添加命令处理程序代码
	if (m_Location.GetLength() == 0)
		return;
	
	m_pHandler->Paste();
	OnRefresh();
}


void CFileManagerDlg::OnSearch()
{
	m_pHandler->Search();
}


void CFileManagerDlg::OnLvnEndlabeleditList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;

	if (m_FileList.m_hWnd){
		int idx = pDispInfo->item.iItem;
		CString s, strNewFileName;
		s += m_FileList.GetItemText(idx, 0);
		s += TEXT("\n");
		m_FileList.GetEditControl()->GetWindowText(strNewFileName);
		s += strNewFileName;
		//
		if (strNewFileName != m_oldFileName){
			m_pHandler->Rename(s.GetBuffer());

			OnRefresh();
		}
	}
}

void CFileManagerDlg::OnCancel()
{
}


int CALLBACK CFileManagerDlg::CompareByName(LPARAM idx1, LPARAM idx2, LPARAM data)
{
	CFileManagerDlg * pWnd = (CFileManagerDlg*)data;

	CString s1 = pWnd->m_FileList.GetItemText(idx1, 0);
	CString s2 = pWnd->m_FileList.GetItemText(idx2, 0);

	int ret = (s1 < s2) ? -1 : 1;
	return ret * pWnd->m_order;
}

int CALLBACK CFileManagerDlg::CompareByType(LPARAM idx1, LPARAM idx2, LPARAM data)
{
	CFileManagerDlg * pWnd = (CFileManagerDlg*)data;

	CString s1 = pWnd->m_FileList.GetItemText(idx1, 1);
	CString s2 = pWnd->m_FileList.GetItemText(idx2, 1);

	int ret = (s1 < s2) ? -1 : 1;
	return ret * pWnd->m_order;
}

int CALLBACK CFileManagerDlg::CompareByDate(LPARAM idx1, LPARAM idx2, LPARAM data)
{
	CFileManagerDlg * pWnd = (CFileManagerDlg*)data;

	CString s1 = pWnd->m_FileList.GetItemText(idx1, 3);
	CString s2 = pWnd->m_FileList.GetItemText(idx2, 3);

	int ret = (s1 < s2) ? -1 : 1;
	return ret * pWnd->m_order;
}

int CALLBACK CFileManagerDlg::CompareByFreeSize(LPARAM idx1, LPARAM idx2, LPARAM data)
{
	CFileManagerDlg * pWnd = (CFileManagerDlg*)data;

	CString s1 = pWnd->m_FileList.GetItemText(idx1, 4);
	CString s2 = pWnd->m_FileList.GetItemText(idx2, 4);

	int ret = (s1 < s2) ? -1 : 1;
	return ret * pWnd->m_order;
}
void CFileManagerDlg::OnLvnColumnclickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;

	if (pNMLV->iSubItem != m_LastSortColum)
	{
		m_LastSortColum = pNMLV->iSubItem;
		m_order = 1;
	}
	else
	{
		m_order = -m_order;
	}

	switch (pNMLV->iSubItem)
	{
	case 0:
		m_FileList.SortItemsEx(CompareByName, (DWORD)this);
		break;
	case 1:
		m_FileList.SortItemsEx(CompareByType, (DWORD)this);
		break;
	case 3:
		m_FileList.SortItemsEx(CompareByDate, (DWORD)this);
		break;
	case 4:
		m_FileList.SortItemsEx(CompareByFreeSize, (DWORD)this);
	default:
		break;
	}

}
