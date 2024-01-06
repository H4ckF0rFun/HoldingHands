#include "stdafx.h"
#include "OutputWnd.h"


COutputWnd::COutputWnd()
{
}


COutputWnd::~COutputWnd()
{

}
BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	
	CRect dummyRect;

	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	dummyRect.SetRectEmpty();

	if (!m_Log.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, dummyRect, this, 1))
	{
		TRACE0("未能创建List Ctrl\n");
		return -1;      
	}


	//CMFCListCtrl
	return 0;
}


void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(rect);
	m_Log.MoveWindow(rect);
	m_Log.RedrawWindow();

	RedrawWindow();
}
