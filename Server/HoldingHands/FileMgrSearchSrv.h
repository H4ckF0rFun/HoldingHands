#pragma once

#include "file_search_common.h"
#include "EventHandler.h"




//Notify Message
// CFileMgrSearchDlg 对话框
#define WM_FILE_MGR_SEARCH_FOUND		(WM_USER + 195)
#define WM_FILE_MGR_SEARCH_OVER			(WM_USER + 196)
#define WM_FILE_MGR_SEARCH_ERROR		(WM_USER + 197)

class CFileMgrSearchSrv :
	public CEventHandler
{
public:
	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数
	//有数据到达的时候调用这两个函数.
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

	void OnFound(TCHAR*Buffer);
	void OnOver();

	void Search(TCHAR*szParams);
	void Stop();

	CFileMgrSearchSrv(CClient *pClient);
	~CFileMgrSearchSrv();
};

