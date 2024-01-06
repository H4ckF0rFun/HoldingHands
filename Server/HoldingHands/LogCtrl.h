#pragma once
#include "afxlistctrl.h"
class CLogCtrl :
	public CMFCListCtrl
{

public:
	CLogCtrl();
	virtual ~CLogCtrl();

	void Log(CONST TCHAR * logMsg);

	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
};

