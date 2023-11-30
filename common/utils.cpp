#include "stdafx.h"
#include "utils.h"



#pragma comment(lib,"Shlwapi.lib")

BOOL MakesureDirExist(const TCHAR* Path, BOOL bIncludeFileName = FALSE)
{
	TCHAR* pTempDir = StrDup(Path);
	BOOL   bResult  = FALSE;
	TCHAR* pIt     = NULL;

	//找到文件名.;
	if (bIncludeFileName){
		pIt = pTempDir + lstrlen(pTempDir) - 1;
		while (pIt[0] != '\\' && pIt[0] != '/' && pIt > pTempDir) pIt--;
		if (pIt[0] != '/' && pIt[0] != '\\')
			goto Return;
		//'/' ---> 0
		pIt[0] = 0;
	}
	//找到':';
	if ((pIt = StrStr(pTempDir, TEXT(":"))) == NULL || (pIt[1] != '\\' && pIt[1] != '/'))
		goto Return;
	pIt++;

	while (pIt[0]){
		char oldCh;
		//跳过'/'或'\\';
		while (pIt[0] && (pIt[0] == '\\' || pIt[0] == '/'))
			pIt++;
		//找到结尾.;
		while (pIt[0] && (pIt[0] != '\\' && pIt[0] != '/'))
			pIt++;
		//
		oldCh = pIt[0];
		pIt[0] = 0;

		if (!CreateDirectory(pTempDir, NULL) && 
			GetLastError() != ERROR_ALREADY_EXISTS)
			goto Return;

		pIt[0] = oldCh;
	}
	bResult = TRUE;
Return:
	LocalFree(pTempDir);
	return bResult;
}

static TCHAR * MemUnits[] =
{
	TEXT("B"),
	TEXT("KB"),
	TEXT("MB"),
	TEXT("GB"),
	TEXT("TB")
};

void GetStorageSizeString(const LARGE_INTEGER & Bytes, TCHAR* strBuffer){

	double bytes = Bytes.QuadPart;
	int MemUnitIdx = 0;

	while (bytes > 1024 && MemUnitIdx <
		((sizeof(MemUnits) / sizeof(MemUnits[0])) - 1)
		)
	{
		MemUnitIdx++;
		bytes /= 1024;
	}

	UINT32 integer_part = bytes;
	UINT32 float_part = 0;
	
	if(bytes >= integer_part)
	{
		float_part = 100 * (bytes - integer_part);
	}

	wsprintf(strBuffer, TEXT("%u.%02u %s"), integer_part, float_part, MemUnits[MemUnitIdx]);

}


#include <Tlhelp32.h.>


DWORD getProcessId(TCHAR * szProcessName){
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof PROCESSENTRY32;
	DWORD pid = 0;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	BOOL bFind = Process32First(hSnap, &pe);
	while (bFind) 
	{
		if (!lstrcmpi(pe.szExeFile, szProcessName))
		{
			pid = pe.th32ProcessID;
			break;
		}
		bFind = Process32Next(hSnap, &pe);
	}
	CloseHandle(hSnap);
	return pid;
}

DWORD getParentProcessId(TCHAR * szProcessName){
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof PROCESSENTRY32;
	DWORD ppid = 0;
	DWORD pid = getProcessId(szProcessName);

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	BOOL bFind = Process32First(hSnap, &pe);
	while (bFind) 
	{
		if (pe.th32ProcessID == pid)
		{
			ppid = pe.th32ParentProcessID;
			break;
		}
		bFind = Process32Next(hSnap, &pe);
	}
	CloseHandle(hSnap);
	return ppid;
}


DWORD getParentProcessId(DWORD dwPid){
	PROCESSENTRY32 pe;
	DWORD ppid = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	BOOL bFind;

	pe.dwSize = sizeof PROCESSENTRY32;
	bFind = Process32First(hSnap, &pe);
	while (bFind) 
	{
		if (pe.th32ProcessID == dwPid)
		{
			ppid = pe.th32ParentProcessID;
			break;
		}
		bFind = Process32Next(hSnap, &pe);
	}
	CloseHandle(hSnap);
	return ppid;
}


char* convertUTF8ToAnsi(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}

char* convertAnsiToUTF8(const char* gb2312)
{
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}


/* 获取exe 所在的目录 */
void GetProcessDirectory(TCHAR * szPath)
{
	HMODULE hMod = GetModuleHandleA(0);
	TCHAR szModuleFileName[MAX_PATH];
	TCHAR * it = NULL;
	szPath[0] = '\0';

	GetModuleFileName(hMod, szModuleFileName, MAX_PATH);
	it = szModuleFileName + lstrlen(szModuleFileName) - 1;

	while (it >= szModuleFileName && *it != '\\')
	{
		--it;
	}
	if (it >= szModuleFileName)
	{
		*it = '\0';
		lstrcpy(szPath, szModuleFileName);
	}
}

float StrToFloat(const TCHAR * str)
{
	UINT32 integer_part;
	UINT32 float_part = 0;
	const TCHAR *p = str;
	UINT32 k = 1;

	integer_part = StrToInt(str);

	while (*p >= '0' && *p <= '9') ++p;

	if (*p == 0 || *p != '.')
	{
		return integer_part;
	}
	//*p != 0,and *p = '.'
	++p;
	float_part = StrToInt(p);

	while (*p >= '0' && *p <= '9')
	{
		k *= 10;
		++p;
	}
	return ((float)integer_part) + ((float)float_part) / k;
}

BOOL DebugPrivilege(const TCHAR *PName, BOOL bEnable)
{
	BOOL              bResult = TRUE;
	HANDLE            hToken;
	TOKEN_PRIVILEGES  TokenPrivileges;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		bResult = FALSE;
		return bResult;
	}
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

	LookupPrivilegeValue(NULL, PName, &TokenPrivileges.Privileges[0].Luid);
	AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		bResult = FALSE;
	}

	CloseHandle(hToken);
	return bResult;
}
