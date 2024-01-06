#pragma once
#include "afxdockablepane.h"
#include "LogCtrl.h"

class COutputWnd :
	public CDockablePane
{

private:
	CLogCtrl m_Log;

public:
	void log(TCHAR * log_str);

	CLogCtrl * GetLogCtrl() { return &m_Log; };

	COutputWnd();
	virtual ~COutputWnd();
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

