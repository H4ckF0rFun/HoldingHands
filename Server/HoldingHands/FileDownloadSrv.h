#pragma once
#include "EventHandler.h"
#include "filedownloader_common.h"
#include <WinInet.h>

//Notify Message
#define WM_MNDD_FILEINFO			(WM_USER + 701)
#define WM_MNDD_DOWNLOAD_RESULT		(WM_USER + 702)
#define WM_MNDD_ERROR				(WM_USER + 703)


class CFileDownloadSrv :
	public CEventHandler
{
public:

	/*
	0.Ok
	1.InternetReadFileFailed.
	2.WriteFileFailed.
	3.Finished.
	*/
private:
	BOOL			m_DownloadFinished;
	CFileDownloadSrv*m_pDlg;
public:
	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数

	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);

	void OnFileInfo(char * FileInfo);
	void OnDownloadResult(char * result);

	CFileDownloadSrv(CClient*pClient);

	~CFileDownloadSrv();
};

