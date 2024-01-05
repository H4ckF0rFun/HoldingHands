#pragma once
#include "chat_common.h"
#include "EventHandler.h"

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

	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnOpen();				//��socket���ӵ�ʱ������������

	//�����ݵ����ʱ���������������.
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;


	void OnChatInit(DWORD dwRead, char*szBuffer);
	void OnChatMsg(DWORD dwRead, char*szbuffer);

	//
	void SendText(TCHAR *szMsge);
	CChatSrv(CClient*pClient);
	~CChatSrv();
};

