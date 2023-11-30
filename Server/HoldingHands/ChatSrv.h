#pragma once
#include "EventHandler.h"

#define CHAT		('C'|('H'<<8)|('A'<<16)|('T'<<24))

//handler message..
#define CHAT_INIT			(0xaa00)			//client---svr
#define CHAT_BEGIN			(0xaa01)			//svr---client
#define CHAT_MSG			(0xaa02)			//p2p

//notify message
#define WM_CHAT_ERROR			(WM_USER + 101)
#define WM_CHAT_GET_NICKNAME	(WM_USER + 102)			
#define WM_CHAT_BEGIN			(WM_USER + 103)
#define WM_CHAT_MSG				(WM_USER + 104)

class CChatSrv :
	public CEventHandler
{
public:
	TCHAR		m_szNickName[128];

	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数

	//有数据到达的时候调用这两个函数.
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;


	void OnChatInit(DWORD dwRead, char*szBuffer);
	void OnChatMsg(DWORD dwRead, char*szbuffer);

	//
	void SendText(TCHAR *szMsge);
	CChatSrv(CClient*pClient);
	~CChatSrv();
};

