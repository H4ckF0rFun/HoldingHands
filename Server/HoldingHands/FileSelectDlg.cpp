// FileSelectDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "FileSelectDlg.h"
#include "afxdialogex.h"


// CFileSelectDlg �Ի���

IMPLEMENT_DYNAMIC(CFileSelectDlg, CDialogEx)

CFileSelectDlg::CFileSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFileSelectDlg::IDD, pParent)
	, m_FileList(_T(""))
{
}

CFileSelectDlg::~CFileSelectDlg()
{
}

void CFileSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCSHELLLIST1, m_List);
	DDX_Control(pDX, IDC_MFCSHELLTREE1, m_Tree);
	DDX_Control(pDX, IDC_EDIT1, m_SelFiles);
}


BEGIN_MESSAGE_MAP(CFileSelectDlg, CDialogEx)
	
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MFCSHELLLIST1, &CFileSelectDlg::OnLvnItemchangedMfcshelllist1)
	ON_NOTIFY(LVN_DELETEALLITEMS, IDC_MFCSHELLLIST1, &CFileSelectDlg::OnLvnDeleteallitemsMfcshelllist1)
	ON_BN_CLICKED(IDOK, &CFileSelectDlg::OnBnClickedOk)
	ON_NOTIFY(TVN_SELCHANGED, IDC_MFCSHELLTREE1, &CFileSelectDlg::OnTvnSelchangedMfcshelltree1)
END_MESSAGE_MAP()


// CFileSelectDlg ��Ϣ�������



BOOL CFileSelectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	
	m_List.DeleteAllItems();

	m_List.ModifyStyle(LVS_TYPEMASK, LVS_ICON );			//���????
	m_Tree.SetRelatedList(&m_List);
	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}


void CFileSelectDlg::OnLvnItemchangedMfcshelllist1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	CString Text;
	//��ѡ����
	
	CString FilePath;
	CString FileName(FilePath);

	if (m_List.m_hWnd && pNMLV->iItem >= 0){
		m_List.GetItemPath(FilePath, pNMLV->iItem);			//Ҫ��ȡ��չ��....
		if (FilePath.GetLength()){
			TCHAR * pFileName = FilePath.GetBuffer() + FilePath.GetLength() - 1;
			while (pFileName >= FilePath.GetBuffer() && *pFileName != '\\'){
				--pFileName;
			}
			if (pFileName >= FilePath.GetBuffer()){
				FileName = (pFileName + 1);
			}
		}
	}

	if (!(pNMLV->uOldState &LVIS_SELECTED ) && (pNMLV->uNewState &LVIS_SELECTED))
	{
		m_MapFileList.SetAt(FileName, 0);
		POSITION pos = m_MapFileList.GetStartPosition();
		while (pos){
			CString Key; void* value;
			m_MapFileList.GetNextAssoc(pos, Key, value);
			Text += TEXT("\"") + Key + TEXT("\" ");
		}
		m_SelFiles.SetWindowText(Text);
	}
	//��ȡ����
	else if (!(pNMLV->uNewState &LVIS_SELECTED )&& (pNMLV->uOldState &LVIS_SELECTED))
	{
		m_MapFileList.RemoveKey(FileName);
		POSITION pos = m_MapFileList.GetStartPosition();
		while (pos){
			CString Key; void* value;
			m_MapFileList.GetNextAssoc(pos, Key, value);
			Text += TEXT("\"") + Key + TEXT("\"");
		}
		m_SelFiles.SetWindowText(Text);
	}
}


void CFileSelectDlg::OnLvnDeleteallitemsMfcshelllist1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	//��û�д���edit֮ǰҲ������������mmp.
	if (m_SelFiles.m_hWnd){
		m_MapFileList.RemoveAll();
		m_SelFiles.SetWindowText(TEXT(""));
	}
}


void CFileSelectDlg::OnBnClickedOk()
{
	if (m_List.GetCurrentFolder(m_CurrentDirectory))
	{
		/*
			Ŀ¼
			�ļ�1
			�ļ�2
			....
		*/

		m_FileList = TEXT("");

		POSITION pos = m_MapFileList.GetStartPosition();
		while (pos)
		{
			CString Key; void* value;
			m_MapFileList.GetNextAssoc(pos, Key, value);
			m_FileList += Key;
			m_FileList += TEXT("\n");
		}

		if (m_FileList.GetLength())
		{
			MessageBox(m_FileList, TEXT("You has selected these files:"));
		}		
	}
	else
		m_FileList = TEXT("");		//û��·��

	CDialogEx::OnOK();
}


void CFileSelectDlg::OnTvnSelchangedMfcshelltree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
}
