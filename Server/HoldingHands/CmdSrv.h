#pragma once
#include "EventHandler.h"
#include "cmd_common.h"


//notify
#define WM_CMD_BEGIN	(WM_USER + 125)
#define WM_CMD_RESULT	(WM_USER + 126)
#define WM_CMD_ERROR	(WM_USER + 127)

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

