#pragma once

#include "chat_common.h"
#include "EventHandler.h"
#include "module.h"


class CChat :
	public CEventHandler
{
private:
	Module *    m_owner;
	TCHAR		m_szPeerName[256];
	HANDLE		m_hWndThread;				//线程句柄;
	DWORD		m_dwThreadId;				//线程ID;
	HWND		m_hDlg;
	HANDLE		m_hInit;
public:
	
	void OnClose();							//所有模块应该在该函数里面就把相关资源清理调.因为相关的析构函数不会被调用;
	void OnOpen();

	void OnEvent(UINT32 e, BYTE* lpData, UINT Size);

	//开始chat;
	void OnChatBegin(DWORD dwRead, TCHAR*szNickName);
	void OnChatMsg(DWORD dwRead, TCHAR*szMsg);
	BOOL	ChatInit();
	//
	static void ThreadProc(CChat*pChat);
	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CChat(CClient *pClient,Module  * owner);
	~CChat();
};

