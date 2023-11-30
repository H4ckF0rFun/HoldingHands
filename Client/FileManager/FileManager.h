#pragma once
#include "EventHandler.h"

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

class CFileManager:
	public CEventHandler
{
	typedef void (*pModuleEntry)(char* szServerAddr,unsigned short uPort,DWORD dwParam);
private:

	typedef struct DriverInfo{
		TCHAR szName[128];
		TCHAR szTypeName[128];
		TCHAR szFileSystem[128];
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
		TCHAR szFileName[4];
	}FmFileInfo;

private:
	TCHAR*		m_pCurDir;			//µ±Ç°Ä¿Â¼.
	//copy / cut
	TCHAR*		m_SrcPath;
	TCHAR*		m_FileList;
	DWORD		m_bMove;

	BOOL ChDir(const TCHAR* Dir);

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
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);

	static void FMCpOrMvFile(TCHAR*szFileName, BOOL bIsDir, DWORD dwParam);

public:
	void OnChangeDir(TCHAR*Buffer);
	void OnGetCurList();
	void OnUp();
	void OnRefresh();
	void OnSearch();

	void OnUploadFromUrl(TCHAR*FileName);
	void OnUploadFromDisk(TCHAR*FileName);
	void OnDownload(TCHAR*FileName);
	void OnRunFile(DWORD Event, TCHAR*FileName);
	void OnNewFolder();
	void OnRename(TCHAR*FileName);
	void OnDelete(TCHAR*FileName);
	void OnCopy(TCHAR*FileName);
	void OnCut(TCHAR*FileName);
	void OnPaste(TCHAR*FileName);

	CFileManager(CClient*pClient);
	~CFileManager();
};

