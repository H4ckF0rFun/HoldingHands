#ifndef _UTILS_H_
#define _UIILS_H_
#include <windows.h>
#include <stdarg.h>
#include <Shlwapi.h>

#define HEAP_ALLOC(x) HeapAlloc(GetProcessHeap(),0,(x))
#define HEAP_FREE(x)  HeapFree(GetProcessHeap(),0,(x))


BOOL MakesureDirExist(const TCHAR* Path, BOOL bIncludeFileName);

void GetStorageSizeString(const LARGE_INTEGER & Bytes, TCHAR* strBuffer);

char* convertUTF8ToAnsi(const char* utf8);

char* convertAnsiToUTF8(const char* gb2312);

char* convertUtf16ToAnsi(const wchar_t* utf16);

wchar_t* convertAnsiToUtf16(const char* gb2312);

void GetProcessDirectory(TCHAR* szPath);

DWORD getParentProcessId(DWORD dwPid);

DWORD getParentProcessId(TCHAR* szProcessName);

DWORD getProcessId(TCHAR* szProcessName);
BOOL DebugPrivilege(const TCHAR *PName, BOOL bEnable);

float StrToFloat(const TCHAR * str);

#endif