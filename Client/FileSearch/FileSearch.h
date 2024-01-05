#pragma once
#include "EventHandler.h"
#include "file_search_common.h"
#include "SearchFile.h"
#include "module.h"

class CFileSearch :
	public CEventHandler
{
private:
	Module *    m_owner;
	CSearchFile m_searcher;

public:
	void OnClose();
	void OnOpen();
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);
	void OnSearch(TCHAR * SearchArg);
	void OnStop();

	CFileSearch(CClient*pClient, Module *    owner);
	~CFileSearch();
};

