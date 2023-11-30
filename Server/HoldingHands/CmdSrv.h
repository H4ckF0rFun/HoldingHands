#pragma once
#include "EventHandler.h"
#define CMD				('C'|('M')<<8|('D')<<16)

//0�ɹ�,-1ʧ��
#define CMD_BEGIN		(0xcad0)

#define CMD_COMMAND		(0xcad1)
#define CMD_RESULT		(0xcad2)


//notify

#define WM_CMD_BEGIN	(WM_USER + 125)
#define WM_CMD_RESULT	(WM_USER + 126)
#define WM_CMD_ERROR	(WM_USER + 127)


class CCmdWnd;
class CCmdSrv :
	public CEventHandler
{
public:
	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnOpen();				//��socket���ӵ�ʱ������������

	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);

	void OnCmdBegin(DWORD dwStatu);
	void OnCmdResult(TCHAR*szBuffer);
	CCmdSrv(CClient*pClient);
	~CCmdSrv();
};

