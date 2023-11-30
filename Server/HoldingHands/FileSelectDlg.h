#pragma once
#include "afxshelllistctrl.h"
#include "afxshelltreectrl.h"
#include "afxwin.h"


// CFileSelectDlg 对话框

class CFileSelectDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFileSelectDlg)

public:
	CFileSelectDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFileSelectDlg();

// 对话框数据
	enum { IDD = IDD_SELECT_FILE_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CMFCShellListCtrl m_List;
	CMFCShellTreeCtrl m_Tree;

	CString				m_CurrentDirectory;
	CString				m_FileList;
	CMapStringToPtr		m_MapFileList;
	

	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedMfcshelllist1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnDeleteallitemsMfcshelllist1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
	CEdit m_SelFiles;
	afx_msg void OnTvnSelchangedMfcshelltree1(NMHDR *pNMHDR, LRESULT *pResult);
};
