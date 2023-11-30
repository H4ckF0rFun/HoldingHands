#include "stdafx.h"
#include "ChatSrv.h"

CChatSrv::CChatSrv(CClient*pClient) :
CEventHandler(pClient,CHAT)
{
	memset(m_szNickName, 0, sizeof(m_szNickName));
}


CChatSrv::~CChatSrv()
{
	dbg_log("CChatSrv::~CChatSrv()");
}


void CChatSrv::OnOpen()
{
	//Get nick name ..(read from dialog..)
	Notify(WM_CHAT_GET_NICKNAME, (WPARAM)m_szNickName,sizeof(m_szNickName));
}

void CChatSrv::OnClose()
{
	Notify(WM_CHAT_ERROR, (WPARAM)TEXT("Connection close..."));
}



void CChatSrv::OnChatInit(DWORD dwRead, char*szBuffer)
{
	DWORD dwStatu = *(DWORD*)szBuffer;
	if (dwStatu)
	{
		//¿ªÊ¼ÁÄÌì
		Send(CHAT_BEGIN, m_szNickName, sizeof(TCHAR)*(lstrlen(m_szNickName) + 1));
		Notify(WM_CHAT_BEGIN);
	}
	else
	{
		Close();
	}
}

void CChatSrv::OnChatMsg(DWORD dwRead, char*szbuffer)
{
	Notify(WM_CHAT_MSG,(WPARAM)szbuffer,dwRead);
}

void CChatSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case CHAT_INIT:
		OnChatInit(Size, (char*)lpData);
		break;
	case CHAT_MSG:
		OnChatMsg(Size,(char*) lpData);
		break;
	default:
		break;
	}
}



void CChatSrv::SendText(TCHAR *szMsg)
{
	Send(CHAT_MSG, (TCHAR*)szMsg, sizeof(TCHAR) * (lstrlen(szMsg) + 1));
}