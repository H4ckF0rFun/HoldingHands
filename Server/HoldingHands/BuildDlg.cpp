// BuildDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "BuildDlg.h"
#include "afxdialogex.h"


// CBuildDlg 对话框

IMPLEMENT_DYNAMIC(CBuildDlg, CDialogEx)

CBuildDlg::CBuildDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBuildDlg::IDD, pParent)
	, m_ServerIP(_T("127.0.0.1"))
	, m_Port(_T("10086"))
{

}

CBuildDlg::~CBuildDlg()
{
}

void CBuildDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_ServerIP);
	DDX_Text(pDX, IDC_EDIT3, m_Port);
}


BEGIN_MESSAGE_MAP(CBuildDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CBuildDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CBuildDlg 消息处理程序


void CBuildDlg::OnBnClickedOk()
{
	UpdateData(TRUE);
	if (m_ServerIP.GetLength() == NULL ||
		m_Port.GetLength() == NULL){
		return;
	}

	//HINSTANCE hModule = GetModuleHandle(0);
	//HRSRC hRsrc = FindResourceA(hModule, MAKEINTRESOURCE(0), "DATA");
	//if (hRsrc){
	//	HGLOBAL hGlobal = LoadResource(hModule, hRsrc);
	//	
	//	if (hGlobal){
	//		//copy data to buffer;
	//		int len = SizeofResource(hModule, hRsrc);
	//		char*pRes = (char*)LockResource(hGlobal);
	//		char*buffer = (char*)malloc(len);
	//		memcpy(buffer, pRes, len);
	//		UnlockResource(hGlobal);

	//		hModule = (HMODULE)buffer;
	//		//find addr in export table
	//		//都是相对地址，要不然得重定位.
	//		IMAGE_DOS_HEADER*pDosHeader = (IMAGE_DOS_HEADER*)hModule;
	//		IMAGE_NT_HEADERS*pNtHeaders = (IMAGE_NT_HEADERS*)((DWORD)hModule + pDosHeader->e_lfanew);
	//		IMAGE_EXPORT_DIRECTORY*pExportDir = (IMAGE_EXPORT_DIRECTORY*)(pNtHeaders->OptionalHeader.DataDirectory[0].VirtualAddress + (DWORD)hModule);
	//		DWORD*pNameTable = (DWORD*)(pExportDir->AddressOfNames + (DWORD)hModule);
	//		//
	//		//在函数名称表 和 序号表里面的索引是一样的.
	//		for (int i = 0; i < pExportDir->NumberOfNames; i++)
	//		{
	//			char*Name = (char*)((DWORD)hModule + pNameTable[i]);

	//			if (!strcmp(Name,"szServerAddr"))
	//			{
	//				WORD* pOrdTable = (WORD*)(pExportDir->AddressOfNameOrdinals + (DWORD)hModule);
	//				DWORD*pFuncTable = (DWORD*)((DWORD)hModule + pExportDir->AddressOfFunctions);
	//				char*szServerAddr = (char*)(pFuncTable[pOrdTable[i]] + (DWORD)hModule);
	//				strcpy(szServerAddr, m_ServerIP.GetBuffer());
	//			}
	//			if (!strcmp(Name, "Port")){
	//				WORD* pOrdTable = (WORD*)(pExportDir->AddressOfNameOrdinals + (DWORD)hModule);
	//				DWORD*pFuncTable = (DWORD*)((DWORD)hModule + pExportDir->AddressOfFunctions);
	//				DWORD*pPort = (DWORD*)(pFuncTable[pOrdTable[i]] + (DWORD)hModule);
	//				*pPort = atoi(m_Port);
	//			}
	//		}
	//		//save exe file
	//		CFileDialog FileDlg(
	//			FALSE, 
	//			"*.exe", 
	//			"client.exe", 
	//			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
	//			"exe file(*.exe)|*.exe",
	//			this
	//		);
	//		if (IDOK == FileDlg.DoModal())
	//		{
	//			HANDLE hFile = CreateFile(FileDlg.GetPathName(), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	//			DWORD dwWrite = 0;
	//			if (hFile != INVALID_HANDLE_VALUE)
	//			{
	//				WriteFile(hFile, buffer, len, &dwWrite, NULL);
	//				CloseHandle(hFile);
	//			}
	//			if (hFile == INVALID_HANDLE_VALUE || dwWrite != len){
	//				MessageBox("Build client failed!");
	//			}
	//			else{
	//				MessageBox("Build client success!");
	//			}
	//		}
	//		free(buffer);
	//	}
	//}
	CDialogEx::OnOK();
}
