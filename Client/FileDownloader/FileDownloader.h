#pragma once
#include "EventHandler.h"
#include "filedownloader_common.h"
#include <WinInet.h>
#include "module.h"

class CFileDownloader :
	public CEventHandler
{
	/*
		0.Ok
		1.InternetReadFileFailed.
		2.WriteFileFailed.
		3.Finished.
	*/
private:
	Module  * m_owner;


	HINTERNET	m_hInternet;
	HINTERNET	m_hConnect;
	HINTERNET	m_hRemoteFile;
	HANDLE		m_hLocalFile;

	BOOL	m_DownloadSuccess;

	int		m_iTotalSize;
	int		m_iFinishedSize;

	URL_COMPONENTS url;

	BYTE *	 m_lpBuffer;

	UINT32   m_flags;
	TCHAR*	 m_szLocalFileName;
	TCHAR *   m_szUrl;

	TCHAR*	 m_szUrlPath;
	TCHAR*	 m_szHost;
	TCHAR*	 m_szUser;
	TCHAR*	 m_szPassword;
	TCHAR*	 m_szExtraInfo;

public:

	void OnOpen();
	void OnClose();
	void OnEvent(UINT32 e, BYTE* lpData, UINT Size);

	CFileDownloader(
		CClient* pClient,
		Module * owner,
		UINT32 flags,
		const TCHAR * saveDir,
		const TCHAR * url);

	void OnGetFileInfo();
	void OnContinueDownload();
	void OnEndDownload();

	~CFileDownloader();
};

