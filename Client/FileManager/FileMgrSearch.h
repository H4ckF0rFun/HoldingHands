#pragma once
#include "EventHandler.h"
#include "SearchFile.h"
#define FILEMGR_SEARCH			('S'|('R'<<8)|('C'<<16)|('H'<<24))

#define FILE_MGR_SEARCH_SEARCH		(0xaca1)

#define FILE_MGR_SEARCH_STOP		(0xaca2)

#define FILE_MGR_SEARCH_FOUND		(0xaca3)

#define FILE_MGR_SEARCH_OVER		(0xaca4)

class CFileMgrSearch :
	public CEventHandler
{
private:

	CSearchFile m_searcher;

public:
	void OnClose();
	void OnOpen();
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);
	void OnSearch(TCHAR * SearchArg);
	void OnStop();

	CFileMgrSearch(CClient*pClient);
	~CFileMgrSearch();
};

