#pragma once
#include "afxcmn.h"
#include "resource.h"




// CFileManagerDlg 对话框	
class CFileManagerSrv;


class CFileManagerDlg : public CDialogEx
{
	
	typedef struct
	{
		TCHAR szName[128];
		TCHAR szTypeName[128];
		TCHAR szFileSystem[128];
		ULARGE_INTEGER	Total;
		ULARGE_INTEGER	Free;
		DWORD dwType;
	}DriverInfo;


	typedef struct
	{
		DWORD dwFileAttribute;
		DWORD dwFileSizeLo;
		DWORD dwFileSizeHi;
		DWORD dwLastWriteLo;
		DWORD dwLastWriteHi;
		TCHAR szFileName[2];
	}FMFileInfo;


	DECLARE_DYNAMIC(CFileManagerDlg)

public:
	CFileManagerDlg(CFileManagerSrv*pHandler, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFileManagerDlg();

// 对话框数据
	enum { IDD = IDD_FM_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	static HIMAGELIST hImageList_SmallIcon;
	static HIMAGELIST hImageList_LargeIcon;
	static CImageList	*pLargeImageList;		//File List Icon.
	static CImageList	*pSmallImageList;

	CFileManagerSrv*	m_pHandler;
	BOOL				m_DestroyAfterDisconnect;
	int					m_order;
	int					m_LastSortColum;

	CString m_oldFileName;

	virtual BOOL OnInitDialog();
	CListCtrl m_FileList;
	CString m_Location;
	CToolBar	m_ToolBar;
	CImageList	m_ImageList;			//tool bar image list
	
	void Resize();
	void FillDriverList(DriverInfo*pDis,int count);

	void FillFileList(FMFileInfo*pfis);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();

	afx_msg void OnToolbarDropDown(NMHDR* pnmhdr, LRESULT* plRes);
	afx_msg void OnViewIcon();
	afx_msg void OnViewDetailedinformation();
	afx_msg void OnViewList();


	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChDir(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnFillList(WPARAM wParam, LPARAM lParam);

	afx_msg void OnUp();
	afx_msg void OnGo();
	afx_msg void OnRefresh();
	afx_msg void OnSearch();

	afx_msg void OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnUploadFromdisk();
	afx_msg void OnUploadFromurl();
	afx_msg void OnMenuDownload();

	afx_msg LRESULT	OnNewfolderSuccess(WPARAM wParam, LPARAM lParam);

	afx_msg void OnRunfileHide();
	afx_msg void OnRunfileNormal();
	afx_msg void OnMenuRename();
	afx_msg void OnMenuNewfolder();
	virtual void OnOK();
	afx_msg void OnMenuDelete();
	afx_msg void OnMenuCopy();
	afx_msg void OnMenuCut();
	afx_msg void OnMenuPaste();
	afx_msg void OnLvnEndlabeleditList1(NMHDR *pNMHDR, LRESULT *pResult);

	void AddItem(int idx, FMFileInfo * pis);
	virtual void PostNcDestroy();
	virtual void OnCancel();
	afx_msg void OnLvnColumnclickList1(NMHDR *pNMHDR, LRESULT *pResult);


	//compare function
	static int CALLBACK CompareByName(LPARAM idx1, LPARAM idx2, LPARAM data);
	static int CALLBACK CompareByType(LPARAM idx1, LPARAM idx2, LPARAM data);
	static int CALLBACK CompareByDate(LPARAM idx1, LPARAM idx2, LPARAM data);
	static int CALLBACK CompareByFreeSize(LPARAM idx1, LPARAM idx2, LPARAM data);
	//
	
};
