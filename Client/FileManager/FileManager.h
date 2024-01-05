#pragma once
#include "EventHandler.h"
#include "file_manager_common.h"
#include "module.h"


typedef void(*typeRun)(LPVOID kernel, const TCHAR * module_name, const TCHAR* module_entry, Params *lpParams);

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
	Module * m_owner;
	//
	TCHAR*		m_pCurDir;			//µ±Ç°Ä¿Â¼.
	//copy / cut
	TCHAR*		m_SrcPath;
	TCHAR*		m_FileList;
	DWORD		m_bMove;
	//

	LPVOID      m_kernel;
	typeRun     m_run_module;


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

	CFileManager(CClient*pClient,Module * owner,typeRun run,LPVOID kernel);
	~CFileManager();
};

