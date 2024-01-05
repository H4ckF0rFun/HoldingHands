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
	HANDLE		m_hWndThread;				//�߳̾��;
	DWORD		m_dwThreadId;				//�߳�ID;
	HWND		m_hDlg;
	HANDLE		m_hInit;
public:
	
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
	CChat(CClient *pClient,Module  * owner);
	~CChat();
};

