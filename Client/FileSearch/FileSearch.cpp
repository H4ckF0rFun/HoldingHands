#include "FileSearch.h"
#include "utils.h"
#pragma comment(lib,"Shlwapi.lib")

CFileSearch::CFileSearch(CClient*pClient, Module *    owner) :
CEventHandler(pClient, FILEMGR_SEARCH)
{
	m_owner = owner;
	if (m_owner)
		get_module(m_owner);
}



CFileSearch::~CFileSearch()
{
	if (m_owner)
		put_module(m_owner);
}


void CFileSearch::OnClose()
{
	OnStop();
}

void CFileSearch::OnOpen()
{

}


void CFileSearch::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case FILE_MGR_SEARCH_SEARCH:
		OnSearch((TCHAR*)lpData);
		break;
	case FILE_MGR_SEARCH_STOP:
		OnStop();
		break;
	default:
		break;
	}
}


void OnFoundFile(WCHAR* path, WIN32_FIND_DATAW* pfd, LPVOID Param)
{
	//printf("OnFoundFile!");
	struct FindFile
	{
		UINT32 dwFileAttribute;
		WCHAR szFileName[2];
	};
	//找到一个就发送一个.
	CFileSearch*pMgrSearch = (CFileSearch*)Param;
	UINT32   dwLen = sizeof(UINT32) + sizeof(TCHAR)* (lstrlen(path) + 1 + lstrlen(pfd->cFileName) + 1);
	FindFile*pFindFile = (FindFile*)malloc(dwLen);

	pFindFile->dwFileAttribute = pfd->dwFileAttributes;

	lstrcpy(pFindFile->szFileName, path);
	lstrcat(pFindFile->szFileName, TEXT("\n"));
	lstrcat(pFindFile->szFileName, pfd->cFileName);

	pMgrSearch->Send(FILE_MGR_SEARCH_FOUND, (char*)pFindFile, dwLen);
	free(pFindFile);
	//printf("OnFoundFile OK!");
}
void OnSearchOver(LPVOID Param)
{
	CFileSearch*pMgrSearch = (CFileSearch*)Param;
	pMgrSearch->Send(FILE_MGR_SEARCH_OVER, 0, 0);
}

void CFileSearch::OnSearch(TCHAR * SearchArg)
{
	TCHAR*szStartLocation = SearchArg;
	TCHAR*szFileName = StrStr(SearchArg, TEXT("\n"));

	if (!szFileName)
		return;

	*szFileName++ = NULL;
	
	m_searcher.Search(szFileName, szStartLocation, 0, OnFoundFile, OnSearchOver, this);
}
void CFileSearch::OnStop()
{
	m_searcher.StopSearching();
}
