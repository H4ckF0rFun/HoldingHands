#pragma once

#include "Client.h"

class CEventHandler
{
public:
	//don't use Notify in constructor or destructor.
	CEventHandler(CClient * pClient,UINT32 ModuleID);
	virtual ~CEventHandler();

	HWND			m_hNotifyWindow;
	CClient    *    m_pClient;
	UINT32          m_ID;

	//now you can use Notify.
	virtual void OnOpen() = 0;
	virtual void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size) = 0;
	virtual void OnClose() = 0;

	//
	void Close();
	void Get();
	void Put();
	
	void Send(UINT32 e,const void *lpData, UINT32 Size);
	void Send(UINT32 e, vec * Bufs, int nBuf);

	LRESULT	Notify(UINT Message = 0, WPARAM wParam = 0, LPARAM lParam = 0, BOOL bWait = TRUE);

	UINT32 GetModuleID()
	{
		return m_ID;
	}

	UINT GetPeerAddress(char* szIP)
	{
		SOCKADDR_IN addr = { 0 };
		int namelen = sizeof(addr);
		UINT Port = 0;

		*szIP = 0;

		if (m_pClient->GetPeerName((SOCKADDR*)&addr, &namelen))
		{
			Port = ntohs(addr.sin_port);
			lstrcpyA(szIP, inet_ntoa(addr.sin_addr));
		}
		return Port;
	}

	void	SetNotifyWindow(HWND hWnd = NULL)
	{
		m_hNotifyWindow = hWnd;
	}
};

