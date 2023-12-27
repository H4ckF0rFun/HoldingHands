#pragma once
#include "afxwin.h"


class CProcessManagerSrv;
class CProcessManagerWnd :
	public CFrameWnd
{

private:
	CProcessManagerSrv* m_pHandler;

	//
	CListCtrl			m_ProcessList;
	CStatusBar          m_wndStatusBar;

	DWORD				m_LastSortColum;
	int					m_order;
	CImageList			m_ImageList;
public:
	
	CProcessManagerWnd(CProcessManagerSrv*pHandler);
	virtual ~CProcessManagerWnd();
	DECLARE_MESSAGE_MAP()

	afx_msg LRESULT OnUpdateCpuUsage(WPARAM wParam, LPARAM lParma);
	afx_msg LRESULT OnUpdateMemUsage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateProcessCount(WPARAM wParam, LPARAM lParam);

	//
	afx_msg LRESULT OnAppendProcessInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRemoveProcessInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateProcessInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam); 


	afx_msg LRESULT OnGrowImageList(WPARAM wParam, LPARAM lParam);

	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnPopMenu(NMHDR*pNMHDR, LRESULT * pResult);

	static int CALLBACK CProcessManagerWnd::CompareByName(LPARAM lParam1,
		LPARAM lParam2,
		LPARAM lParamSort);

	static int CALLBACK CProcessManagerWnd::CompareByUser(LPARAM lParam1,
		LPARAM lParam2,
		LPARAM lParamSort);

	static int CALLBACK CProcessManagerWnd::CompareByPid(LPARAM lParam1,
		LPARAM lParam2,
		LPARAM lParamSort);

	static int CALLBACK CProcessManagerWnd::CompareByPPid(LPARAM lParam1,
		LPARAM lParam2,
		LPARAM lParamSort);


	static int CALLBACK CProcessManagerWnd::CompareByCPU(LPARAM lParam1,
		LPARAM lParam2,
		LPARAM lParamSort);

	static int CALLBACK CProcessManagerWnd::CompareByHandle(LPARAM lParam1,
		LPARAM lParam2,
		LPARAM lParamSort);

	static int CALLBACK CProcessManagerWnd::CompareByThread(LPARAM lParam1,
		LPARAM lParam2,
		LPARAM lParamSort);

	void OnColumClick(NMHDR*pNMHDR, LRESULT * pResult);
	afx_msg void OnProcessKillprocess();
	afx_msg void OnProcessParent();
};

