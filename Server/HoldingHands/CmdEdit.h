#pragma once
#include "afxwin.h"
#include "CmdSrv.h"

class CCmdEdit :
	public CEdit
{
private:
	CCmdSrv*	&m_pHandler;
	int m_ReadOnlyLength;

	CFont		m_Font;

	CList<CString>	m_Commands;
	POSITION		m_LastCommand;

	int			m_LastLineTextLength;
public:
	void OnEnter();
	void OnCmdBegin();
	void OnCmdResult(const CString & strResult);
	CCmdEdit(CCmdSrv*	&pHandler);
	~CCmdEdit();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

