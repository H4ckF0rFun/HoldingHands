#ifndef _LOADER_H
#define _LOADER_H
#include <Windows.h>
#include "spinlock.h"

typedef BOOL(WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

#define FLAG_DEAD 0x1

typedef struct Module
{
	PIMAGE_DOS_HEADER DosHeaders;
	PIMAGE_NT_HEADERS NtHeaders;
	DWORD			  ImageSize;
	BYTE*			  ImageBase;
	BYTE*             OriginalBase;
	DllEntryProc	  Entry;
	
	HMODULE*		  Dependency;
	UINT32            CntOfDependency;

	ULONG			  RefCount;
	//UINT32			  Flags;
	//spinlock_t		  spinlock;

}Module;

#define get_module(x) InterlockedIncrement(&((x)->RefCount))
#define put_module(x) InterlockedDecrement(&((x)->RefCount))

//common
typedef struct Params
{
	void(*release)(struct Params * lpParams);
	UINT32 num;
}Params;


LPVOID __GetProcAddress(HMODULE hModule, const char*ProcName);
Module*  __LoadModule(const BYTE* data);
int      __FreeModule(Module * module);

#endif