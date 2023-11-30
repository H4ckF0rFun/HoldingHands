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
	HANDLE		m_hWndThread;				//�߳̾��;
	DWORD		m_dwThreadId;				//�߳�ID;
	HWND		m_hDlg;
	HANDLE		m_hInit;			
	void OnClose();							//����ģ��Ӧ���ڸú�������Ͱ������Դ�����.��Ϊ��ص������������ᱻ����;
	void OnOpen();

	void OnEvent(UINT32 e, BYTE* lpData, UINT Size);

	//��ʼchat;
	void OnChatBegin(DWORD dwRead, TCHAR*szNickName);
	void OnChatMsg(DWORD dwRead, TCHAR*szMsg);
	BOOL	ChatInit();
	//
	static void ThreadProc(CChat*pChat);
	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CChat(CClient *pClient);
	~CChat();
};

