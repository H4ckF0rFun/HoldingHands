#include "stdafx.h"
#include "FileDownloadSrv.h"
#include "json\json.h"
#include "utils.h"

/*
	需要修改 通知速度.要不然会占用CPU太高.
*/
CFileDownloadSrv::CFileDownloadSrv(CClient*pClient) :
CEventHandler(pClient, MINIDOWNLOAD),
	m_DownloadFinished(FALSE),
	m_pDlg(NULL)
{
}


CFileDownloadSrv::~CFileDownloadSrv()
{

}

void CFileDownloadSrv::OnClose()
{
}

void CFileDownloadSrv::OnOpen()
{
	//发送请求.获取文件大小.
	Send(MNDD_GET_FILE_INFO, 0, 0);
}

void CFileDownloadSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case MNDD_FILE_INFO:
		OnFileInfo((char*)lpData);
		break;
	case MNDD_DOWNLOAD_RESULT:
		OnDownloadResult((char*)lpData);
		break;
	default:
		break;
	}
}


void CFileDownloadSrv::OnDownloadResult(char*result)
{
	Json::Value root;

	if (!Json::Reader().parse(result, root))
	{
		Notify(WM_MNDD_ERROR, (WPARAM)TEXT("Parse Json Failed"));
		Close();
		return;
	}
	
	if (root["code"] != "0")
	{
#ifdef UNICODE 
		TCHAR tmp[MAX_PATH] = { 0 };
		MultiByteToWideChar(CP_ACP,0,root["err"].asCString(),lstrlenA(root["err"].asCString()),tmp,MAX_PATH - 1);
		Notify(WM_MNDD_ERROR, (WPARAM)tmp);
#else
		Notify(WM_MNDD_ERROR, (WPARAM)root["err"].asCString());
#endif
		
		Close();
		return;
	}

	/*
	{
		"code" : "",
		"err" : "",
		"total_size" :
		"finished_size" :
	}
	*/

	LPVOID ArgList[3] = { 0 };
	ArgList[0] = (LPVOID)root["total_size"].asInt();
	ArgList[1] = (LPVOID)root["finished_size"].asInt();
	
	if (root["err"] == "finished")
	{
		ArgList[2] = (LPVOID)1;
	}

	Notify(
		WM_MNDD_DOWNLOAD_RESULT,
		sizeof(ArgList) / sizeof(ArgList[0]),
		(LPARAM)ArgList
		);

	//continue download....
	if (!ArgList[2])
	{
		Send(MNDD_DOWNLOAD_CONTINUE, 0, 0);
	}
	else
	{
		m_DownloadFinished = TRUE;
		Send(MNDD_DOWNLOAD_END, 0, 0);
	}
}

void CFileDownloadSrv::OnFileInfo(char*fileinfo)
{
	Json::Value root;
	if (!Json::Reader().parse(fileinfo, root))
	{
		Notify(WM_MNDD_ERROR, (WPARAM)TEXT("Parse Json Failed"));
		Close();
		return;
	}

	/*
	{
		"code" : ,
		"err" : "",
		"filename" : "",
		"url" : "",
		"filesize": -1,		//-1 表示未知
	}
	*/
	if (root["code"] != "0")
	{
#ifdef UNICODE 
		TCHAR tmp[MAX_PATH] = { 0 };
		MultiByteToWideChar(CP_ACP, 0, root["err"].asCString(), lstrlenA(root["err"].asCString()), tmp, MAX_PATH - 1);
		Notify(WM_MNDD_ERROR, (WPARAM)tmp);
#else
		Notify(WM_MNDD_ERROR, (WPARAM)root["err"].asCString());
#endif

		Close();
		return;
	}

	LPVOID ArgList[3];
	ArgList[2] = (LPVOID)root["filesize"].asInt();

#ifdef UNICODE 
	//
	
	TCHAR filename[MAX_PATH] = { 0 };
	TCHAR url[MAX_PATH] = { 0 };

	MultiByteToWideChar(CP_ACP, 0, root["filename"].asCString(), lstrlenA(root["filename"].asCString()), filename, MAX_PATH - 1);
	MultiByteToWideChar(CP_ACP, 0, root["url"].asCString(), lstrlenA(root["url"].asCString()), url, MAX_PATH - 1);

	
	ArgList[0] = filename;
	ArgList[1] = url;

	Notify(
		WM_MNDD_FILEINFO,
		sizeof(ArgList) / sizeof(ArgList[0]),
		(LPARAM)ArgList
		);

#else
	ArgList[0] = root["filename"].asCString();
	ArgList[1] = root["url"].asCString();

	Notify(
		WM_MNDD_FILEINFO,
		sizeof(ArgList) / sizeof(ArgList[0]),
		(LPARAM)ArgList
		);
#endif

	

	if (root["filesize"].asInt() != 0)
	{
		Send(MNDD_DOWNLOAD_CONTINUE, 0, 0);
	}
}