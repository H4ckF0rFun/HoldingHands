#ifndef KNEL_COMMON_H
#define KNEL_COMMON_H

#define KNEL	('K'|('N'<<8)|('E'<<16)|('L'<<24))

#include <stdint.h>
#include <WinSock2.h>

/*******************************EVENT ID**************************/
struct Params;

enum KNEL_EVENT
{
	KNEL_LOGIN = 0x0,
	KNEL_READY,
	KNEL_POWER_REBOOT,
	KNEL_POWER_SHUTDOWN,
	KNEL_EDITCOMMENT,
	KNEL_EDITCOMMENT_OK,
	KNEL_RESTART,
	KNEL_GETMODULE_INFO,
	KNEL_MODULE_INFO,
	KNEL_MODULE_CHUNK_GET,
	KNEL_MODULE_CHUNK_DAT,
	KNEL_ERROR,
	KNEL_UTILS_COPYTOSTARTUP,
	KNEL_UTILS_WRITE_REG,
	KNEL_UTILS_OPEN_WEBPAGE,
	KNEL_RUN_MODULE,
	KNEL_EXIT
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

/*
	kernel param.
		//
		release
		num

		//
		ArgList



		//
		ParamInfo[0]
		ParamInfo[1]
*/

#define PARAM_FLAG_RELEASE 0x1

enum ParamType
{
	type_value,
	type_pointer
};

struct ParamInfo
{
	uint8_t type;
	uint8_t flag;
};

void kernel_params_release(Params * lpParams);
Params * deserialize_kernel_params(BYTE * lpData, UINT Size);
UINT32 serialize_kernel_params(BYTE ** lppOutData, int argc, ...);
#endif
