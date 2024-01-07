#include "stdafx.h"
#include "LogCtrl.h"


CLogCtrl::CLogCtrl()
{
}


CLogCtrl::~CLogCtrl()
{
}
BEGIN_MESSAGE_MAP(CLogCtrl, CMFCListCtrl)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CLogCtrl::OnLvnColumnclick)
	ON_WM_SIZE()
END_MESSAGE_MAP()


int CLogCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	DWORD dwExStyle = 0;

	if (CMFCListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	//set log style..
	ModifyStyle(LVS_SINGLESEL, 0);

	dwExStyle = GetExStyle()
		| LVS_EX_FULLROWSELECT
		| LVS_EX_AUTOCHECKSELECT
		| LVS_EX_HEADERDRAGDROP;

	SetExtendedStyle(dwExStyle);

	InsertColumn(0, TEXT("Date"), LVCFMT_LEFT, 250);
	InsertColumn(1, TEXT("Event"), LVCFMT_LEFT,800);		//OK

	return 0;
}


void CLogCtrl::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 1;
}

void CLogCtrl::Log(CONST TCHAR * logMsg)
{
	int id = GetItemCount();
	InsertItem(id, CTime::GetCurrentTime().Format("[%Y-%m-%d %H:%M:%S]"));
	SetItemText(id, 1, logMsg);
}
