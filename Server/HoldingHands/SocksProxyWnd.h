#pragma once
#include "afxwin.h"
#include "resource.h"



class ClientCtx;
class CSocksProxySrv;
class CSocksProxyWnd :
	public CFrameWnd
{
	
public:
	CSocksProxySrv * m_pHandler;
	BOOL			 m_IsRunning;

	CListCtrl	 m_Connections;

	char			m_ListenAddress[0x100];
	UINT			m_ListenPort;
	char			m_UdpAssociateAddr[0x100];


	CSocksProxyWnd(CSocksProxySrv*pHandler);
	~CSocksProxyWnd();

	CMenu	m_Menu;
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	LRESULT OnError(WPARAM wParam, LPARAM lParam);

	afx_msg void OnVerSocks4();
	afx_msg void OnVerSocks5();
	afx_msg void OnMainStartproxy();
	afx_msg void OnMainStop();
	afx_msg void OnUpdateMainStartproxy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMainStop(CCmdUI *pCmdUI);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg void OnNMRClickList(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnConnectionsDisconnectall();
	afx_msg void OnConnectionsDisconnect();


public:
	LRESULT OnProxyConnected(WPARAM wParam, LPARAM lParam);
	LRESULT OnProxyClosed(WPARAM wParam, LPARAM lParam);
};

