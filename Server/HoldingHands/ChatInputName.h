#pragma once
#include "resource.h"

// CChatInputName 对话框

class CChatInputName : public CDialogEx
{
	DECLARE_DYNAMIC(CChatInputName)

public:
	CChatInputName(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CChatInputName();

// 对话框数据
	enum { IDD = IDD_CHAT_NAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_NickName;
	afx_msg void OnBnClickedOk();
};
