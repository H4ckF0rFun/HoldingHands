#include "stdafx.h"
#include "resource.h"
#include "ProcessManagerSrv.h"
#include "ProcessManagerWnd.h"
#include "utils.h"

CProcessManagerWnd::CProcessManagerWnd(CProcessManagerSrv*pHandler):
m_pHandler(pHandler)
{
	m_pHandler->Get();
	m_LastSortColum = -1;
	m_order = 1;
}


CProcessManagerWnd::~CProcessManagerWnd()
{
	m_pHandler->Put();
}


#define IDC_LIST 2351

BEGIN_MESSAGE_MAP(CProcessManagerWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_RCLICK, IDC_LIST, OnPopMenu)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST, OnColumClick)
	ON_MESSAGE(WM_PROCESS_MANAGER_APPEND,OnAppendProcessInfo)
	ON_MESSAGE(WM_PROCESS_MANAGER_REMOVE,OnRemoveProcessInfo)
	ON_MESSAGE(WM_PROCESS_MANAGER_MODIFY,OnUpdateProcessInfo)
	ON_MESSAGE(WM_PROCESS_MANAGER_CPU_USAGE_MODIFY,OnUpdateCpuUsage)
	ON_MESSAGE(WM_PROCESS_MANAGER_MEMORY_USAGE_MODIFY,OnUpdateMemUsage)
	ON_MESSAGE(WM_PROCESS_MANAGER_PROCESS_COUNT_MODIFY,OnUpdateProcessCount)
	ON_MESSAGE(WM_PROCESS_MANAGER_APPEND_ICON,OnGrowImageList)
	ON_COMMAND(ID_PROCESS_KILLPROCESS, &CProcessManagerWnd::OnProcessKillprocess)
	ON_COMMAND(ID_PROCESS_PARENT, &CProcessManagerWnd::OnProcessParent)
END_MESSAGE_MAP()


static UINT indicators[] =
{
	ID_PROCESS_COUNT,
	ID_CPU_USAGE,
	ID_MEM_USAGE
};

//排序都有一个默认值.

int CALLBACK CProcessManagerWnd::CompareByName(LPARAM lParam1,
	LPARAM lParam2,
	LPARAM lParamSort)
{
	CProcessManagerWnd * pWnd = (CProcessManagerWnd*)lParamSort;

	CString left = pWnd->m_ProcessList.GetItemText(lParam1,0);
	CString right = pWnd->m_ProcessList.GetItemText(lParam2, 0);

	int ret = (left < right) ? -1 : 1;
	return ret * pWnd->m_order;
}

int CALLBACK CProcessManagerWnd::CompareByPid(LPARAM lParam1,
	LPARAM lParam2,
	LPARAM lParamSort)
{
	CProcessManagerWnd * pWnd = (CProcessManagerWnd*)lParamSort;
	return (lParam1 - lParam2) * pWnd->m_order;
}

int CALLBACK CProcessManagerWnd::CompareByPPid(LPARAM lParam1,
	LPARAM lParam2,
	LPARAM lParamSort)
{
	CProcessManagerWnd * pWnd = (CProcessManagerWnd*)lParamSort;

	CString left = pWnd->m_ProcessList.GetItemText(lParam1, 2);
	CString right = pWnd->m_ProcessList.GetItemText(lParam2, 2);
	
	return -1 * (StrToInt(left) - StrToInt(right)) * pWnd->m_order;
}

int CALLBACK CProcessManagerWnd::CompareByUser(LPARAM lParam1,
	LPARAM lParam2,
	LPARAM lParamSort)
{
	CProcessManagerWnd * pWnd = (CProcessManagerWnd*)lParamSort;
	CString left  = pWnd->m_ProcessList.GetItemText(lParam1, 3);
	CString right = pWnd->m_ProcessList.GetItemText(lParam2, 3);

	int ret = (left < right) ? -1 : 1;
	return ret * pWnd->m_order;
}

int CALLBACK CProcessManagerWnd::CompareByCPU(LPARAM lParam1,
	LPARAM lParam2,
	LPARAM lParamSort)
{
	CProcessManagerWnd * pWnd = (CProcessManagerWnd*)lParamSort;
	CString left  = pWnd->m_ProcessList.GetItemText(lParam1, 4);
	CString right = pWnd->m_ProcessList.GetItemText(lParam2, 4);

	return -100 * (StrToFloat(left) - StrToFloat(right)) * pWnd->m_order;
}


int CALLBACK CProcessManagerWnd::CompareByHandle(LPARAM lParam1,
	LPARAM lParam2,
	LPARAM lParamSort)
{
	CProcessManagerWnd * pWnd = (CProcessManagerWnd*)lParamSort;
	CString left  = pWnd->m_ProcessList.GetItemText(lParam1, 6);
	CString right = pWnd->m_ProcessList.GetItemText(lParam2, 6);

	return -1 * (StrToInt(left) - StrToInt(right)) * pWnd->m_order;

}


int CALLBACK CProcessManagerWnd::CompareByThread(LPARAM lParam1,
	LPARAM lParam2,
	LPARAM lParamSort)
{
	CProcessManagerWnd * pWnd = (CProcessManagerWnd*)lParamSort;
	CString left  = pWnd->m_ProcessList.GetItemText(lParam1, 7);
	CString right = pWnd->m_ProcessList.GetItemText(lParam2, 7);

	return -1 * (StrToInt(left) - StrToInt(right)) * pWnd->m_order;
}


void CProcessManagerWnd:: OnColumClick(NMHDR*pNMHDR, LRESULT * pResult)
{
	NM_LISTVIEW * pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (pNMListView->iSubItem != m_LastSortColum)
	{
		m_order = 1;
	}
	else
	{
		m_order = -m_order;
	}

	m_LastSortColum = pNMListView->iSubItem;

	switch (pNMListView->iSubItem)
	{
	case 0:
		m_ProcessList.SortItemsEx(CompareByName, (DWORD)this);
		break;
	case 1:
		m_ProcessList.SortItems(CompareByPid, (DWORD)this);
		break;
	case 2:
		m_ProcessList.SortItemsEx(CompareByPPid, (DWORD)this);
		break;
	case 3:
		m_ProcessList.SortItemsEx(CompareByUser, (DWORD)this);
		break;
	case 4:
		m_ProcessList.SortItemsEx(CompareByCPU, (DWORD)this);
		break;
	
	case 6:
		m_ProcessList.SortItemsEx(CompareByHandle, (DWORD)this);
		break;
	case 7:
		m_ProcessList.SortItemsEx(CompareByThread, (DWORD)this);
		break;
	default:
		break;
	}
	*pResult = 0;
	return;
}


void CProcessManagerWnd::OnClose()
{
	m_pHandler->SetNotifyWindow();
	m_pHandler->Close();
	DestroyWindow();
}


int CProcessManagerWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// TODO:  在此添加您专用的创建代码
	CRect   rect;
	CString Text;
	char    szIP[0x100];
	UINT32  colunm = 0;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	//创建状态栏
	if (!m_wndStatusBar.Create(this)){
		TRACE0("未能创建StatuBar\n");
		return -1;      // 未能创建
	}

	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetPaneInfo(0, ID_PROCESS_COUNT, SBPS_NORMAL, 110);
	m_wndStatusBar.SetPaneInfo(1, ID_CPU_USAGE, SBPS_NORMAL, 110);
	m_wndStatusBar.SetPaneInfo(2, ID_MEM_USAGE, SBPS_NORMAL, 110);


	GetClientRect(&rect);

	if (!m_ProcessList.Create(
		WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT,
		rect,
		this,
		IDC_LIST)
		)
	{
		TRACE0("未能创建ProcessList\n");
		return -1;      // 未能创建
	}

	DWORD dwExStyle = m_ProcessList.GetExStyle();
	m_ProcessList.ModifyStyle(LVS_SINGLESEL, 0);
	dwExStyle |= LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_AUTOCHECKSELECT ;
	m_ProcessList.SetExtendedStyle(dwExStyle);

	//左对齐
	m_ProcessList.InsertColumn(colunm++, TEXT("Name"), LVCFMT_LEFT, 160);
	m_ProcessList.InsertColumn(colunm++, TEXT("PID"), LVCFMT_LEFT, 80);				//OK
	m_ProcessList.InsertColumn(colunm++, TEXT("ParentPID"), LVCFMT_LEFT, 100);		//OK
	m_ProcessList.InsertColumn(colunm++, TEXT("User"), LVCFMT_LEFT, 125);			//OK
	m_ProcessList.InsertColumn(colunm++, TEXT("CPU"), LVCFMT_LEFT, 80);				//OK
	m_ProcessList.InsertColumn(colunm++, TEXT("Mem(WorkSet)"), LVCFMT_LEFT, 110);	//OK
	m_ProcessList.InsertColumn(colunm++, TEXT("Handle"), LVCFMT_LEFT, 120);			//--
	m_ProcessList.InsertColumn(colunm++, TEXT("Thread"), LVCFMT_LEFT, 120);			//--
	m_ProcessList.InsertColumn(colunm++, TEXT("Path"), LVCFMT_LEFT, 400);				//--
	//Create Statu bar

	m_ProcessList.MoveWindow(&rect);

	//
	m_ImageList.Create(16, 16, ILC_COLOR32, 0, 1024);
	m_ProcessList.SetImageList(&m_ImageList, LVSIL_SMALL);

	//set Text
	m_pHandler->GetPeerAddress(szIP);
	Text.Format(TEXT("[%s] ProcessManager"), CString(szIP));
	SetWindowText(Text);

	//move window...
	GetWindowRect(rect);
	rect.right = rect.left + 980;
	rect.bottom = rect.top + 460;
	MoveWindow(rect);

	//Set Notify Window,,
	m_pHandler->SetNotifyWindow(GetSafeHwnd());

	return 0;
}


void CProcessManagerWnd::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	// TODO:  在此处添加消息处理程序代码
	CRect   statuBarRect;
	CRect	rect;
	GetClientRect(rect);

	m_wndStatusBar.GetClientRect(&statuBarRect);

	rect.bottom -= statuBarRect.bottom;
	m_ProcessList.MoveWindow(rect);
	return;
}


//
afx_msg LRESULT CProcessManagerWnd::OnUpdateCpuUsage(WPARAM wParam, LPARAM lParma)
{
	CString text;
	text.Format(TEXT("CPU: %d %%"), wParam);
	m_wndStatusBar.SetPaneText(1, text);
	return 0;
}

afx_msg LRESULT CProcessManagerWnd::OnUpdateMemUsage(WPARAM wParam, LPARAM lParam)
{
	CString text;
	text.Format(TEXT("Memory: %d %%"), wParam);
	m_wndStatusBar.SetPaneText(2, text);
	return 0;
}

afx_msg LRESULT CProcessManagerWnd::OnUpdateProcessCount(WPARAM wParam, LPARAM lParam)
{
	CString text;
	text.Format(TEXT("Process: %d"), wParam);
	m_wndStatusBar.SetPaneText(0, text);
	return 0;
}

//
afx_msg LRESULT CProcessManagerWnd::OnAppendProcessInfo(WPARAM wParam, LPARAM lParam)
{
	ProcessInfo * Pi = (ProcessInfo*)wParam;
	CString		  Text;
	LARGE_INTEGER liValue;
	TCHAR		  buff[0x100];
	UINT32		  colunm = 1;

	int idx = m_ProcessList.GetItemCount();

	int ret = m_ProcessList.InsertItem(idx, Pi->szName,Pi->dwIconIndex);

	Text.Format(TEXT("%d"), Pi->dwPid);
	m_ProcessList.SetItemText(idx, colunm++, Text);				//pid

	Text.Format(TEXT("%d"), Pi->dwParentPid);
	m_ProcessList.SetItemText(idx, colunm++, Text);				//pid

	m_ProcessList.SetItemText(idx, colunm++, Pi->szUser);		//User

	wsprintf(buff, TEXT("%d.%02d %%"), Pi->dwCPUUsage / 1000, (Pi->dwCPUUsage % 1000) / 10);
	m_ProcessList.SetItemText(idx, colunm++, buff);				//Cpu

	liValue.QuadPart = Pi->dwWorkSet * 1024;
	GetStorageSizeString(liValue, buff);
	m_ProcessList.SetItemText(idx, colunm++, buff);				//Mem

	Text.Format(TEXT("%d"), Pi->dwHandles);
	m_ProcessList.SetItemText(idx, colunm++, Text);				//Handle

	Text.Format(TEXT("%d"), Pi->dwThreads);
	m_ProcessList.SetItemText(idx, colunm++, Text);				//Thread
	
	m_ProcessList.SetItemText(idx, colunm++, Pi->szExePath);	//Thread

	//Save Process Id.
	m_ProcessList.SetItemData(idx, Pi->dwPid);
	return 0;
}

afx_msg LRESULT CProcessManagerWnd::OnRemoveProcessInfo(WPARAM wParam, LPARAM lParam)
{
	LVFINDINFO info = { 0 };
	info.flags = LVFI_PARAM;
	info.lParam = wParam;					//Client ID;
	
	int idx = m_ProcessList.FindItem(&info);
	if (idx >= 0)
	{
		m_ProcessList.DeleteItem(idx);
	}
	return 0;
}

afx_msg LRESULT CProcessManagerWnd::OnUpdateProcessInfo(WPARAM wParam, LPARAM lParam)
{

	ProcessInfo * Pi = (ProcessInfo*)wParam;
	CString       Text;
	TCHAR         buff[0x100];
	LVFINDINFO    info = { 0 };
	int			  idx;
	LARGE_INTEGER liValue;

	info.flags = LVFI_PARAM;
	info.lParam = Pi->dwPid;					//Client ID;

	idx = m_ProcessList.FindItem(&info);
	
	if (idx < 0)
		return 0;
	
	if (lParam & MODIFY_UPDATE_NAME)
		m_ProcessList.SetItemText(idx, 0, Pi->szName); 

	if (lParam & MODIFY_UPDATE_PATH)
		m_ProcessList.SetItemText(idx, 8, Pi->szExePath);	//Path
	
	if (lParam & MODIFY_UPDATE_USER)
		m_ProcessList.SetItemText(idx, 3, Pi->szUser);		//User

	if (lParam & MODIFY_UPDATE_PID)
	{
		Text.Format(TEXT("%d"), Pi->dwPid);
		m_ProcessList.SetItemText(idx, 1, Text);			//pid

		Text.Format(TEXT("%d"), Pi->dwPid);
		m_ProcessList.SetItemText(idx, 2, Text);			//pid
		m_ProcessList.SetItemData(idx, Pi->dwParentPid);

		m_ProcessList.SetItemData(idx, Pi->dwPid);
	}
	
	if (lParam & MODIFY_UPDATE_CPU)
	{
		wsprintf(buff, TEXT("%d.%02d %%"), Pi->dwCPUUsage / 1000, (Pi->dwCPUUsage % 1000) / 10);
		m_ProcessList.SetItemText(idx, 4, buff);			//Cpu
	}

	if (lParam & MODIFY_UPDATE_MEM)
	{
		liValue.QuadPart = Pi->dwWorkSet * 1024;
		GetStorageSizeString(liValue, buff);
		m_ProcessList.SetItemText(idx, 5, buff);			//Mem
	}
	
	if (lParam & MODIFY_UPDATE_HADS)
	{
		Text.Format(TEXT("%d"), Pi->dwHandles);
		m_ProcessList.SetItemText(idx, 6, Text);			//Handle
	}
	
	if (lParam & MODIFY_UPDATE_THDS)
	{
		Text.Format(TEXT("%d"), Pi->dwThreads);
		m_ProcessList.SetItemText(idx, 7, Text);			//Thread
	}
	
	//Update Icon..
	if (lParam & MODIFY_UPDATE_ICON)
		m_ProcessList.SetItem(
			idx, 
			0, 
			LVIF_IMAGE, 
			NULL,
			Pi->dwIconIndex,
			0, 
			0,
			0);

	return 0;
}

LRESULT CProcessManagerWnd::OnGrowImageList(WPARAM wParam, LPARAM lParam)
{
	m_ImageList.Add((HICON)wParam);
	return 0;
}

afx_msg void CProcessManagerWnd::OnPopMenu(NMHDR*pNMHDR, LRESULT * pResult)
{
	POINT			 pt;
	CMenu			 menu;
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	//
	*pResult = 0;

	//设置
	menu.LoadMenu(IDR_PROCESS_MANAGER);
	GetCursorPos(&pt);

	menu.GetSubMenu(0)->TrackPopupMenu(
		TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
		pt.x,
		pt.y,
		this);
}

void CProcessManagerWnd::OnProcessKillprocess()
{
	
	POSITION pos = m_ProcessList.GetFirstSelectedItemPosition();
	int		 idx = 0;

	//kill select process.
	while (pos){
		idx = m_ProcessList.GetNextSelectedItem(pos);
		m_pHandler->KillProcess(m_ProcessList.GetItemData(idx));
	}
}


void CProcessManagerWnd::OnProcessParent()
{
	POSITION   pos = m_ProcessList.GetFirstSelectedItemPosition();
	int		   idx = 0;
	int		   parent_idx = 0;
	UINT       ParentPid = 0;
	LVFINDINFO info = { 0 };
	if (pos)
	{
		idx = m_ProcessList.GetNextSelectedItem(pos);
		ParentPid = StrToInt(m_ProcessList.GetItemText(idx, 2));

		info.flags = LVFI_PARAM;
		info.lParam = ParentPid;					//Client ID;

		parent_idx = m_ProcessList.FindItem(&info);
		if (parent_idx >= 0)
		{
			CRect rc;
			int  nItem = m_ProcessList.GetTopIndex();
			m_ProcessList.GetItemRect(nItem, rc, LVIR_BOUNDS);
			CSize sz(0, (parent_idx - nItem)*rc.Height());
			m_ProcessList.Scroll(sz);
			m_ProcessList.SetItemState(parent_idx, LVIS_SELECTED, LVIS_SELECTED);
			m_ProcessList.SetItemState(idx, 0, LVIS_SELECTED);
		}
	}
}
