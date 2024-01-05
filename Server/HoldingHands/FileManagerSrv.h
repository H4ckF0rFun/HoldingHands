#pragma once
#include "EventHandler.h"
#include "file_manager_common.h"


//notify message
#define WM_FMDLG_CHDIR					(WM_USER + 133)
#define WM_FMDLG_LIST					(WM_USER + 134)
#define WM_FMDLG_TEST					(WM_USER + 135)
#define WM_FMDLG_NEWFOLDER_SUCCESS		(WM_USER + 140)
#define WM_FMDLG_ERROR					(WM_USER + 141)


class CFileManagerSrv :
	public CEventHandler
{
public:
	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();					//当socket连接的时候调用这个函数

	//有数据到达的时候调用这两个函数.
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

	void OnChangeDirRet(DWORD dwRead,char*buffer);
	void OnGetListRet(DWORD dwRead, char*buffer);

	void Echo(WORD Msg);

	void Up();
	void Refresh();
	void NewFolder();
	void Rename(const TCHAR *szNames);
	void ChDir(const TCHAR *szNewDir);
	void Search();

	void Delete(const TCHAR *szFileList);
	void Copy(const TCHAR *szFileList);
	void Cut(const TCHAR *szFileList);
	void Paste();

	void UploadFromUrl(const TCHAR *szUrl);
	
	void UploadFromDisk(const TCHAR * SrcDir, const TCHAR *szFileList);
	void Download(const TCHAR * DestDir, const TCHAR *szFileList);

	void RunFileNormal(const TCHAR *szFileList);
	void RunFileHide(const TCHAR *szFileList);

	CFileManagerSrv(CClient*pClient);
	~CFileManagerSrv();
};

