#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"

// CFileTransDlg 对话框
class CFileTransSrv;



class CFileTransDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFileTransDlg)

public:
	CFileTransDlg(CFileTransSrv*pHandler, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFileTransDlg();

// 对话框数据
	enum { IDD = IDD_FILETRANS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	ULONGLONG		m_ullTotalSize;
	ULONGLONG		m_ullFinishedSize;

	ULONGLONG		m_ullCurrentFinishedSize;
	ULONGLONG		m_ullCurrentFileSize;

	DWORD			m_dwTotalCount;
	DWORD			m_dwFinishedCount;
	DWORD			m_dwFailedCount;

	BOOL			m_TransferFinished;
	CFileTransSrv*m_pHandler;
	CString			m_IP;

	afx_msg void OnClose();

	afx_msg LRESULT OnTransInfo(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnTransFileBegin(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTransFileDC(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTransFileFinished(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT	OnTransFinished(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);

	static CString GetProgressString(ULONGLONG ullFinished, ULONGLONG ullTotal);
	virtual BOOL OnInitDialog();
	CProgressCtrl m_Progress;

	virtual void PostNcDestroy();
	virtual void OnCancel();
	virtual void OnOK();
	CRichEditCtrl m_TransLog;
};
