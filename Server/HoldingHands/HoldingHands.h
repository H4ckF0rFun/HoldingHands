
// HoldingHands.h : HoldingHands 应用程序的主头文件
//
#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"       // 主符号
#include "IOCP.h"

// CHoldingHandsApp:
// 有关此类的实现，请参阅 HoldingHands.cpp
//

class CHoldingHandsApp : public CWinApp
{

private:

	CIOCP * m_Iocp;

public:
	CHoldingHandsApp();
// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// 实现

public:
	CShellManager*	m_pShellManager;
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CHoldingHandsApp theApp;
