#ifndef _LOADER_H
#define _LOADER_H
#include <Windows.h>

LPVOID __GetProcAddress(HMODULE hModule, const char*ProcName);
int __LoadFromMem(const BYTE*image, LPVOID *lppImageBase);
#endif