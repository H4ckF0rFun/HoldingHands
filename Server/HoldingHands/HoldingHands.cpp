
// HoldingHands.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "HoldingHands.h"
#include "MainFrm.h"
#include "utils.h"
#include "ClientDoc.h"
#include "ClientView.h"

extern "C"
{
#include <libavcodec\avcodec.h>
}


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CHoldingHandsApp

BEGIN_MESSAGE_MAP(CHoldingHandsApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CHoldingHandsApp::OnAppAbout)
END_MESSAGE_MAP()


// CHoldingHandsApp ����

CHoldingHandsApp::CHoldingHandsApp()
{
	// ֧����������������
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
	// ���Ӧ�ó��������ù�����������ʱ֧��(/clr)�����ģ���: 
	//     1) �����д˸������ã�������������������֧�ֲ�������������
	//     2) ��������Ŀ�У������밴������˳���� System.Windows.Forms ������á�
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO:  ������Ӧ�ó��� ID �ַ����滻ΪΨһ�� ID �ַ�����������ַ�����ʽ
	//Ϊ CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("HoldingHands.AppID.NoVersion"));

	m_pShellManager = NULL;
	m_Iocp = NULL;
	// TODO:  �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}

// Ψһ��һ�� CHoldingHandsApp ����

CHoldingHandsApp theApp;


// CHoldingHandsApp ��ʼ��

LONG WINAPI dbg_dump(struct _EXCEPTION_POINTERS* ExceptionInfo) {
	char dump[0x1000] = { 0 };
	wsprintfA(dump,
		"CRASH CODE:0x%.8x ADDR=0x%.8x FLAGS=0x%.8x PARAMS=0x%.8x\n"
		"eax=%.8x ebx=%.8x ecx=%.8x\nedx=%.8x esi=%.8x edi=%.8x\neip=%.8x esp=%.8x ebp=%.8x\n",
		ExceptionInfo->ExceptionRecord->ExceptionCode,
		ExceptionInfo->ExceptionRecord->ExceptionAddress,
		ExceptionInfo->ExceptionRecord->ExceptionFlags,
		ExceptionInfo->ExceptionRecord->NumberParameters,
		ExceptionInfo->ContextRecord->Eax,
		ExceptionInfo->ContextRecord->Ebx,
		ExceptionInfo->ContextRecord->Ecx,
		ExceptionInfo->ContextRecord->Edx,
		ExceptionInfo->ContextRecord->Esi,
		ExceptionInfo->ContextRecord->Edi,
		ExceptionInfo->ContextRecord->Eip,
		ExceptionInfo->ContextRecord->Esp,
		ExceptionInfo->ContextRecord->Ebp
		);
	MessageBoxA(NULL, dump, "dbg_dump", MB_OK);
	ExitProcess(0);
	return 0;
}



int worker_thread_init(void * lpParam)
{
	HRESULT hr = CoInitialize(0);
	if (FAILED(hr)){
		return -1;
	}

	return 0;
}

void worker_thread_fini(void * lpParam)
{
	CoUninitialize();
}


BOOL CHoldingHandsApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()��  ���򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	m_pShellManager = new CShellManager;

	// ��ʼ�� OLE ��
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// ʹ�� RichEdit �ؼ���Ҫ  AfxInitRichEdit2()	
	AfxInitRichEdit2();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO:  Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	avcodec_register_all();
	
	//
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 0), &wsadata))
	{
		AfxMessageBox(TEXT("CIOCPServer::SocketInit Failed"));
		return FALSE;
	}
	
	m_Iocp = new CIOCP(0,worker_thread_init,worker_thread_fini,NULL);

	if (!m_Iocp->Create())
	{
		AfxMessageBox(TEXT("m_Iocp->Create() Failed"));
		return FALSE;
	}

	SetUnhandledExceptionFilter(dbg_dump);
#ifdef _DEBUG
	AllocConsole();
	SetConsoleTitleA("HoldingHands Debug Log");
#endif

	//// ��Ҫ���������ڣ��˴��뽫�����µĿ�ܴ���
	//// ����Ȼ��������ΪӦ�ó���������ڶ���
	//CMainFrame* pFrame = new CMainFrame(m_Iocp);
	//if (!pFrame)
	//	return FALSE;

	CSingleDocTemplate * pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CClientDoc),
		RUNTIME_CLASS(CMainFrame),       // �� SDI ��ܴ���
		RUNTIME_CLASS(CClientView)
		);

	if (!pDocTemplate)
		return FALSE;

	AddDocTemplate(pDocTemplate);

	// ������׼ shell ���DDE�����ļ�������������
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// ��������������ָ�������  ���
	// �� /RegServer��/Register��/Unregserver �� /Unregister ����Ӧ�ó����򷵻� FALSE��
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// Ψһ��һ�������ѳ�ʼ���������ʾ����������и���
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CHoldingHandsApp::ExitInstance()
{
	//TODO:  �����������ӵĸ�����Դ
	AfxOleTerm(FALSE);

	if (m_pShellManager){
		delete m_pShellManager;
		m_pShellManager = NULL;
	}
	
	if (m_Iocp)
	{
		m_Iocp->Close();
		m_Iocp->Put();
		m_Iocp = NULL;
	}

	WSACleanup();

	return CWinApp::ExitInstance();
}

// CHoldingHandsApp ��Ϣ�������


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAboutDlg::OnBnClickedOk)
END_MESSAGE_MAP()

// �������жԻ����Ӧ�ó�������
void CHoldingHandsApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CHoldingHandsApp ��Ϣ�������





void CAboutDlg::OnBnClickedOk()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	CDialogEx::OnOK();
}
