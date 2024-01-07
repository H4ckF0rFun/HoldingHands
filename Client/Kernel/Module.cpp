#include "Module.h"
#include "dbg.h"

static int ProtectionFlags[2][2][2] =
{
	{
		// not executable
		{ PAGE_NOACCESS, PAGE_WRITECOPY },
		{ PAGE_READONLY, PAGE_READWRITE },
	}, {
		// executable
		{ PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY },
		{ PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE },
	},
};


LPVOID __GetProcAddress(HMODULE hModule, const char*ProcName)
{
	IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*)(hModule);
	IMAGE_NT_HEADERS *pNtHeaders = (IMAGE_NT_HEADERS*)(pDosHeader->e_lfanew + (DWORD)hModule);
	IMAGE_DATA_DIRECTORY * DataDirectory = (IMAGE_DATA_DIRECTORY*)&pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

	IMAGE_EXPORT_DIRECTORY*pExportDirectory = (IMAGE_EXPORT_DIRECTORY*)(
		DataDirectory->VirtualAddress +
		(DWORD)hModule);

	DWORD dwRavOfExportBegin = DataDirectory->VirtualAddress;
	DWORD dwRvaOfExportEnd = dwRavOfExportBegin + DataDirectory->Size;

	DWORD* FuncTable = (DWORD*)((DWORD)hModule + pExportDirectory->AddressOfFunctions);
	DWORD dwRvaOfFunc = 0;

	//there is no export table;
	if (dwRvaOfExportEnd == dwRavOfExportBegin)
	{
		return NULL;
	}

	if (!(0xffff0000 & (DWORD)ProcName))
	{
		//by ordinary
		WORD ord      = (WORD)ProcName;
		DWORD dwIndex = ord - pExportDirectory->Base;

		if (dwIndex >= pExportDirectory->NumberOfFunctions)
		{
			return 0;
		}
		dwRvaOfFunc = FuncTable[dwIndex];
	}
	else
	{
		//by name
		DWORD * NameTable = (DWORD*)((DWORD)hModule + pExportDirectory->AddressOfNames);
		WORD *	OrdTable  = (WORD*)((DWORD)hModule + pExportDirectory->AddressOfNameOrdinals);

		for (int i = 0; i < pExportDirectory->NumberOfNames; i++)
		{
			char*name = (char*)(NameTable[i] + (DWORD)hModule);
			if (!lstrcmpiA(name, ProcName))
			{
				dwRvaOfFunc = FuncTable[OrdTable[i]];
				break;
			}
		}
	}

	//导出表重定向
	if (dwRvaOfFunc>dwRavOfExportBegin && dwRvaOfFunc < dwRvaOfExportEnd)
	{
		char szBuffer[0x100];
		char*pModule = szBuffer;
		char*pProc   = pModule;
		lstrcpynA(szBuffer, (char*)(dwRvaOfFunc + (DWORD)hModule), 0x100);
		while (pProc[0] && pProc[0] != '.') pProc++;

		*pProc++ = 0;
		
		hModule = GetModuleHandleA(pModule);
		
		//不要load,load 会增加引用计数.
		//

		if (hModule == NULL)
		{
			return 0;
		}
		return __GetProcAddress(hModule, pProc);
	}
	return (void*)(dwRvaOfFunc + (DWORD)hModule);
}

int CopySections(Module * module,const BYTE * data)
{

	IMAGE_SECTION_HEADER * section;
	int NumOfSections    = module->NtHeaders->FileHeader.NumberOfSections;
	DWORD ImageBase = module->ImageBase;

	section = (IMAGE_SECTION_HEADER*)(
		module->ImageBase +
		module->DosHeaders->e_lfanew +
		sizeof(DWORD) +
		sizeof(IMAGE_FILE_HEADER) +
		module->NtHeaders->FileHeader.SizeOfOptionalHeader);

	//展开section,空间已经分配好了,只需要把相应位置的数据copy即可
	for (int i = 0; i < NumOfSections; i++,section++)
	{
		//dbg_log("Section : %p ", section->VirtualAddress);
		if (section->SizeOfRawData == 0) 
		{
			DWORD section_size = module->NtHeaders->OptionalHeader.SectionAlignment;

			if (section_size > 0)
			{
				LPVOID dest = (unsigned char *)VirtualAlloc(
					(LPVOID)(ImageBase + section->VirtualAddress),
					section_size,
					MEM_COMMIT,
					PAGE_READWRITE);

				if (dest == NULL)
				{
					return -1;
				}

				dest = (LPVOID)(ImageBase + section->VirtualAddress);
				memset(dest, 0, section_size);
			}
			// section is empty
			continue;
		}
		else
		{
			//SizeOfRawData == 0, bss
			RtlCopyMemory(
				(LPVOID)(ImageBase + section->VirtualAddress),
				data + section->PointerToRawData,
				section->SizeOfRawData);
		}
	}
	return 0;
}


int Relocate(Module * module)
{
	DWORD     ImageBase = module->ImageBase;
	DWORD     OriginalBase = module->OriginalBase;
	DWORD     Delta;
	PIMAGE_NT_HEADERS NtHeaders = module->NtHeaders;

	IMAGE_BASE_RELOCATION * relocation = (IMAGE_BASE_RELOCATION*)(
		ImageBase + 
		NtHeaders->OptionalHeader.DataDirectory[5].VirtualAddress);

	DWORD relocation_size = NtHeaders->OptionalHeader.DataDirectory[5].Size;

	//don't need relocate.
	if (ImageBase == OriginalBase)
	{
		return 0;
	}

	if (relocation_size == 0)
	{
		return -1;
	}

	Delta = (DWORD)(ImageBase - OriginalBase);
	
	/*
		md 网上的资料都是错的，根本没有结尾空的重定位项标记 结束.
		而是根据重定位表的大小.	
	*/

	//dbg_log("relocation table address : %p", relocation);
	// dbg_log("relocation table size: %x", relocation_size);

	while (relocation_size)
	{
		DWORD dwItems = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2;
		WORD * pAddrs = (WORD*)(sizeof(IMAGE_BASE_RELOCATION) + (DWORD)relocation);

		//dbg_log("relocation addr: %p ,va : %p , size: %d",
		//	relocation,
		//	relocation->VirtualAddress,
		//	relocation->SizeOfBlock);

		relocation_size -= relocation->SizeOfBlock;

		for (int i = 0; i < dwItems; i++)
		{
			WORD dwType = (pAddrs[i] >> 12);
			WORD dwOffset =  (pAddrs[i] & 0xfff);
			DWORD*pRelocationAddr = (DWORD*)(ImageBase + relocation->VirtualAddress + dwOffset);
			//
			switch (dwType)
			{
			case IMAGE_REL_BASED_ABSOLUTE:		//block alignment
				break;
			case IMAGE_REL_BASED_HIGHLOW:
				*pRelocationAddr += Delta;
				break;
			default:
				return -1;
			}
		}
		//Next Block
		relocation = (IMAGE_BASE_RELOCATION*)(relocation->SizeOfBlock + (DWORD)relocation);
	}
	return 0;
}

int FixImport(Module * module)
{
	DWORD ImageBase = module->ImageBase;
	PIMAGE_NT_HEADERS NtHeaders = module->NtHeaders;
	PIMAGE_DATA_DIRECTORY DataDir = NtHeaders->OptionalHeader.DataDirectory;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
	//修复IAT
	//IMAGE_IMPORT_DESCRIPTOR 保存了dll name和从该dll导入哪些函数
	ImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)(
		ImageBase +
		DataDir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for (; ImportDescriptor->Characteristics; ImportDescriptor++)
	{
		uintptr_t * thunkRef = NULL;
		FARPROC *   funcRef  = NULL;
		const char* szModuleName = (const char*)ImageBase + ImportDescriptor->Name;
		HMODULE     hModule  = LoadLibraryA(szModuleName);			//不论模块是否存在,都要load,增加引用计数.

		if (hModule == NULL)
		{
			return -1;
		}

		//add to Dependency.
		if (!module->Dependency)
		{
			module->Dependency = (HMODULE*)HeapAlloc(
				GetProcessHeap(),
				0,
				sizeof(HMODULE) * (module->CntOfDependency + 1));
		}
		else
		{
			module->Dependency = (HMODULE*)HeapReAlloc(
				GetProcessHeap(),
				0,
				module->Dependency,
				sizeof(HMODULE) * (module->CntOfDependency + 1));
		}

		module->Dependency[module->CntOfDependency++] = hModule;

		if (ImportDescriptor->OriginalFirstThunk) {
			thunkRef = (uintptr_t *)(ImageBase + ImportDescriptor->OriginalFirstThunk);
			funcRef = (FARPROC *)(ImageBase + ImportDescriptor->FirstThunk);
		}
		else
		{
			// no hint table
			thunkRef = (uintptr_t *)(ImageBase + ImportDescriptor->FirstThunk);
			funcRef = (FARPROC *)   (ImageBase + ImportDescriptor->FirstThunk);
		}

		for (; *thunkRef; thunkRef++, funcRef++)
		{
			if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) 
			{
				*funcRef = (FARPROC)__GetProcAddress(hModule, (LPCSTR)IMAGE_ORDINAL(*thunkRef));
			}
			else
			{
				PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)(ImageBase + (*thunkRef));
				*funcRef = (FARPROC)__GetProcAddress(hModule, (LPCSTR)&thunkData->Name);
			}
			
			if (*funcRef == 0)
			{
				return -1;
			}
		}
	}
	return 0;
}

Module * AllocModule()
{
	Module * module = (Module*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Module));
	return module;
}


int FinalizeSections(Module* module)
{
	BOOL				  executable;
	BOOL				  readable;
	BOOL				  writeable;
	DWORD			      ImageBase = module->ImageBase;
	PIMAGE_NT_HEADERS     NtHeaders = module->NtHeaders;
	PIMAGE_SECTION_HEADER section   = IMAGE_FIRST_SECTION(NtHeaders);
	
	//set section property
	for (int i = 0; i < module->NtHeaders->FileHeader.NumberOfSections; i++, section++)
	{
		DWORD protect = 0;
		DWORD old_protect = 0;
		DWORD section_size = 0;
	
		// determine protection flags based on characteristics
		executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
		readable =   (section->Characteristics & IMAGE_SCN_MEM_READ) != 0;
		writeable =  (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;

		protect = ProtectionFlags[executable][readable][writeable];

		if (section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
		{
			protect |= PAGE_NOCACHE;
		}

		//get section size
		if ((i + 1) == NtHeaders->FileHeader.NumberOfSections)
			section_size = NtHeaders->OptionalHeader.SizeOfImage - section->VirtualAddress;
		else
			section_size = section[1].VirtualAddress - section->VirtualAddress;

		section->Misc.PhysicalAddress = (DWORD)(ImageBase + section->VirtualAddress);
		
		//protect.
		if (!VirtualProtect(
			(LPVOID)(ImageBase + section->VirtualAddress),
			section_size,
			protect,
			&old_protect))
		{
			return -1;
		}
	}
	return 0;
}

Module*  __LoadModule(const BYTE* data)
{
	//绝对地址需要重定位,所以DosHeader 和 FileHeader 里面的都是相对地址
	IMAGE_DOS_HEADER *       pOldDosHeader = (IMAGE_DOS_HEADER*)data;
	IMAGE_NT_HEADERS *       pOldNtHeaders;
	DWORD					 dwNTHeaderOffset;

	Module * module = NULL;

	dwNTHeaderOffset = pOldDosHeader->e_lfanew;
	pOldNtHeaders = (IMAGE_NT_HEADERS*)(data + dwNTHeaderOffset);

	if (pOldDosHeader->e_magic != IMAGE_DOS_SIGNATURE ||
		pOldNtHeaders->Signature != IMAGE_NT_SIGNATURE)
	{
		SetLastError(ERROR_BAD_EXE_FORMAT);
		goto failed;
	}

	module = AllocModule();
	
	module->ImageSize = pOldNtHeaders->OptionalHeader.SizeOfImage;
	module->OriginalBase = pOldNtHeaders->OptionalHeader.ImageBase;

	module->ImageBase = (DWORD)VirtualAlloc(
			(LPVOID)pOldNtHeaders->OptionalHeader.ImageBase,
			module->ImageSize,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READWRITE);
	

	if (module->ImageBase == NULL)
	{
		module->ImageBase = (DWORD)VirtualAlloc(
			NULL, 
			module->ImageSize,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READWRITE);

		if (module->ImageBase == NULL)
		{
			SetLastError(ERROR_OUTOFMEMORY);
			goto failed;
		}
	}

	module->DosHeaders = (IMAGE_DOS_HEADER*)module->ImageBase;

	//copy Headers (Dos Stub + PE Header + Section Headers)
	RtlCopyMemory(
		(LPVOID)module->ImageBase,
		data, 
		pOldNtHeaders->OptionalHeader.SizeOfHeaders);
	
	//reset pNtHeaders
	module->NtHeaders = (IMAGE_NT_HEADERS*)(module->ImageBase + dwNTHeaderOffset);
	module->NtHeaders->OptionalHeader.ImageBase = module->ImageBase;

	//Copy Sections.
	if (CopySections(module, data))
		goto failed;

	//relocate.
	if (Relocate(module))
		goto failed;

	//fix import table.
	if (FixImport(module))
		goto failed;

	//tls......

	//...
	if (FinalizeSections(module))
		goto failed;

	module->Entry = (DllEntryProc)(
		module->NtHeaders->OptionalHeader.ImageBase +
		module->NtHeaders->OptionalHeader.AddressOfEntryPoint);

	//call dll main,init cruntime
	if (module->Entry)
	{
		module->Entry((HINSTANCE)module->ImageBase, DLL_PROCESS_ATTACH, 0);
	}

	return module;
failed:
	//Clean up.
	if (module)
		__FreeModule(module);

	return NULL;
}

int      __FreeModule(Module * module)
{
	if (InterlockedDecrement(&module->RefCount))
	{
		InterlockedIncrement(&module->RefCount);
		return -1;
	}
	
	//call fini
	if (module->Entry)
		module->Entry((HINSTANCE)module->ImageBase, DLL_PROCESS_DETACH, NULL);

	//free libraries.
	for (int i = 0; i < module->CntOfDependency; i++)
	{
		FreeLibrary(module->Dependency[i]);
	}

	if (module->ImageBase)
		VirtualFree((HINSTANCE)module->ImageBase, 0, MEM_RELEASE);
	//
	HeapFree(GetProcessHeap(), 0, module);
	return 0;
}