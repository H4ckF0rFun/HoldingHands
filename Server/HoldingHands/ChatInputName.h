#pragma once
#include "resource.h"

// CChatInputName �Ի���

class CChatInputName : public CDialogEx
{
	DECLARE_DYNAMIC(CChatInputName)

public:
	CChatInputName(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CChatInputName();

// �Ի�������
	enum { IDD = IDD_CHAT_NAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CString m_NickName;
	afx_msg void OnBnClickedOk();
};
