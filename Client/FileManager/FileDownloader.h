#pragma once
#include "EventHandler.h"
#include <WinInet.h>
#define MINIDOWNLOAD	('M'|('N'<<8)|('D'<<16)|('D'<<24))



#define MNDD_GET_FILE_INFO		(0xabc0)
#define MNDD_FILE_INFO			(0xabd1)

#define MNDD_DOWNLOAD_CONTINUE	(0xabd3)

//成功,创建文件失败.;
#define MNDD_DOWNLOAD_RESULT	(0xabd4)

#define MNDD_DOWNLOAD_END		(0xabd6)


#define FILEDOWNLOADER_FLAG_RUNAFTERDOWNLOAD (0x1)

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

	CFileDownloader(CClient* pClient, UINT32 flags,
		const TCHAR * saveDir, const TCHAR * url);

	void OnGetFileInfo();
	void OnContinueDownload();
	void OnEndDownload();

	~CFileDownloader();
};

