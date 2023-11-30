#include "stdafx.h"
#include "FileMgrSearchSrv.h"

CFileMgrSearchSrv::CFileMgrSearchSrv(CClient *pClient) :
CEventHandler(pClient, FILEMGR_SEARCH)
{
}


CFileMgrSearchSrv::~CFileMgrSearchSrv()
{
}

void CFileMgrSearchSrv::OnClose()
{
}
void CFileMgrSearchSrv::OnOpen()
{
}

//有数据到达的时候调用这两个函数.

void CFileMgrSearchSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case FILE_MGR_SEARCH_FOUND:
		OnFound((TCHAR*)lpData);
		break;
	case FILE_MGR_SEARCH_OVER:
		OnOver();
		break;
	default:
		break;
	}
}

void CFileMgrSearchSrv::OnFound(TCHAR*Buffer)
{	
	Notify(WM_FILE_MGR_SEARCH_FOUND, (WPARAM)Buffer);
}
void CFileMgrSearchSrv::OnOver()
{
	Notify(WM_FILE_MGR_SEARCH_OVER);
}

void CFileMgrSearchSrv::Search(TCHAR*szParams)
{
	Send(FILE_MGR_SEARCH_SEARCH, (TCHAR*)szParams, (sizeof(TCHAR) * (lstrlen(szParams) + 1)));
}

void CFileMgrSearchSrv::Stop()
{
	Send(FILE_MGR_SEARCH_STOP, 0, 0);
}