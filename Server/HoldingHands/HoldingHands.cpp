
// HoldingHands.cpp : 定义应用程序的类行为。
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


// CHoldingHandsApp 构造

CHoldingHandsApp::CHoldingHandsApp()
{
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
	// 如果应用程序是利用公共语言运行时支持(/clr)构建的，则: 
	//     1) 必须有此附加设置，“重新启动管理器”支持才能正常工作。
	//     2) 在您的项目中，您必须按照生成顺序向 System.Windows.Forms 添加引用。
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO:  将以下应用程序 ID 字符串替换为唯一的 ID 字符串；建议的字符串格式
	//为 CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("HoldingHands.AppID.NoVersion"));

	m_pShellManager = NULL;
	m_Iocp = NULL;
	// TODO:  在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}

// 唯一的一个 CHoldingHandsApp 对象

CHoldingHandsApp theApp;


// CHoldingHandsApp 初始化

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
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。  否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	m_pShellManager = new CShellManager;

	// 初始化 OLE 库
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// 使用 RichEdit 控件需要  AfxInitRichEdit2()	
	AfxInitRichEdit2();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO:  应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

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

	//// 若要创建主窗口，此代码将创建新的框架窗口
	//// 对象，然后将其设置为应用程序的主窗口对象
	//CMainFrame* pFrame = new CMainFrame(m_Iocp);
	//if (!pFrame)
	//	return FALSE;

	CSingleDocTemplate * pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CClientDoc),
		RUNTIME_CLASS(CMainFrame),       // 主 SDI 框架窗口
		RUNTIME_CLASS(CClientView)
		);

	if (!pDocTemplate)
		return FALSE;

	AddDocTemplate(pDocTemplate);

	// 分析标准 shell 命令、DDE、打开文件操作的命令行
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// 调度在命令行中指定的命令。  如果
	// 用 /RegServer、/Register、/Unregserver 或 /Unregister 启动应用程序，则返回 FALSE。
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// 唯一的一个窗口已初始化，因此显示它并对其进行更新
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CHoldingHandsApp::ExitInstance()
{
	//TODO:  处理可能已添加的附加资源
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

// CHoldingHandsApp 消息处理程序


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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

// 用于运行对话框的应用程序命令
void CHoldingHandsApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CHoldingHandsApp 消息处理程序





void CAboutDlg::OnBnClickedOk()
{
	// TODO:  在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}
