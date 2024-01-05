#pragma once



#include "EventHandler.h"
#include "cmd_common.h"
#include "module.h"

class CCmd :
	public CEventHandler
{

private:
	Module *    m_owner;
	HANDLE		m_hReadThread;		

	HANDLE		m_hReadPipe;
	HANDLE		m_hWritePipe;
	
	HANDLE		m_hCmdProcess;

	PROCESS_INFORMATION m_pi;

public:
	void OnClose();		
	void OnOpen();

	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);
	int CmdBegin();

	static void __stdcall CCmd::ReadThread(CCmd*pCmd);

	void OnCommand(TCHAR*szCmd);
	CCmd(CClient *pClient,Module * owner);
	~CCmd();
};

