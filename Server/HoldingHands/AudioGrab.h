#pragma once
#include <windef.h>
#include <mmsystem.h>
#include <list>

using std::list;
using std::pair;


#define BUFF_COUNT 16
#define LEN_PER_BUFF 0x10000

class CAudioGrab
{

private:
	DWORD		  m_dwBufSize;
	list<pair<char*, DWORD>> m_buffer_list;
	
	HANDLE		  m_hEvent;
	HANDLE		  m_hMutex;

	HWAVEIN		  m_hWaveIn;		//输入设备

	volatile long m_bStop;

	WAVEFORMATEX	 m_Wavefmt;	//格式
	
	void  RemoveHead(char**ppBuffer, DWORD*pLen);
	void  AddTail(char*Buffer, DWORD dwLen);

	BOOL		m_IsWorking;
	WAVEHDR		m_hdrs[BUFF_COUNT];
	char		m_Buffers[BUFF_COUNT][LEN_PER_BUFF];	//
	UINT		m_Idx;

	HANDLE		m_hWorkThread;
	DWORD		m_dwThreadID;
	static void __stdcall WorkThread(CAudioGrab*pThis);

public:
	BOOL	InitGrabber();
	void	CloseGrabber();
	BOOL	IsWorking()	{ return m_IsWorking; }
	BOOL	GetBuffer(void**ppBuffer,DWORD*pBufLen);

	CAudioGrab();
	~CAudioGrab();
};

