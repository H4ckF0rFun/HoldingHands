#pragma once
#include "afxwin.h"


// CChatDlg �Ի���
class CChatSrv;

class CChatDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CChatDlg)

public:
	CChatDlg(CChatSrv*pHandler, CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CChatDlg();

// �Ի�������
	enum { IDD = IDD_CHAT_DLG };


	CChatSrv*		m_pHandler;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParma);
	afx_msg LRESULT OnChatMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetNickName(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChatBegin(WPARAM wParam, LPARAM lParam);
	
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	CString m_Msg;
	CEdit m_MsgList;
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual void OnCancel();
};
