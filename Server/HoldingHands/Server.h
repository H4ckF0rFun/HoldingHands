#pragma once
#include "TCPSocket.h"
#include "spinlock.h"

class CClient;

#define WM_SOCK_ACCEPTED   (WM_USER + 101)

class CServer :
	public CTCPSocket
{

private:
	HWND	    m_hNotifyWindow;

public:
	CServer();
	LRESULT Notify(DWORD Msg,WPARAM wParam = 0,LPARAM lParam = 0,BOOL Sync = TRUE);

	void SetNotifyWindow(HWND hWnd) { m_hNotifyWindow = hWnd; };

	virtual void OnAccept(
		SOCKET hClientSocket, 
		const SOCKADDR* RemoteAddr, 
		int AddrLen, 
		void * lpParam,
		DWORD Error);

	virtual ~CServer();
};

