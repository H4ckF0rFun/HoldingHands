#pragma once
#include "EventHandler.h"

#define CHAT		('C'|('H'<<8)|('A'<<16)|('T'<<24))

#define CHAT_INIT		(0xaa00)			//client---svr;
#define CHAT_BEGIN		(0xaa01)			//svr---client;
#define CHAT_MSG		(0xaa02)			//p2p;


class CChat :
	public CEventHandler
{
public:
	TCHAR		m_szPeerName[256];
	HANDLE		m_hWndThread;				//线程句柄;
	DWORD		m_dwThreadId;				//线程ID;
	HWND		m_hDlg;
	HANDLE		m_hInit;			
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
	CChat(CClient *pClient);
	~CChat();
};

