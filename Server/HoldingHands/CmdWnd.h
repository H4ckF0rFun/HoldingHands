#pragma once
#include "afxwin.h"
#include "CmdEdit.h"


class CCmdSrv;


class CCmdWnd :
	public CFrameWnd
{
public:
	CCmdWnd(CCmdSrv*pHandler);
	~CCmdWnd();



	CCmdEdit		m_CmdShow;
	CBrush			m_BkBrush;
	CCmdSrv			*m_pHandler;

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	LRESULT OnCmdBegin(WPARAM wParam, LPARAM lParam);
	LRESULT OnError(WPARAM wParam, LPARAM lParam);
	LRESULT OnCmdResult(WPARAM wParam, LPARAM lParam);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

