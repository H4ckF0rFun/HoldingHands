#pragma once
#include "EventHandler.h"
#include "AudioGrab.h"
#include "AudioPlay.h"

#define AUDIO		('A'|'U'<<8|'D'<<16|'O'<<24)


#define AUDIO_ERROR (0x0000)

#define AUDIO_BEGIN (0xaaa0)
#define AUDIO_STOP  (0xaaa2)

#define AUDIO_PLAY_BEGIN	(0xaaa5)
#define AUDIO_PLAY_DATA		(0xaaa6)
#define AUDIO_PLAY_STOP		(0xaaa7)

class CAudio :
	public CEventHandler
{
	CAudioGrab m_AudioGrab;
	CAudioPlay m_AudioPlay;

	HANDLE	   m_hWorkThread;
	volatile long m_IsWorking;

	void OnAudioBegin();
	void OnAudioStop();
	
	//²¥·ÅÉùÒô.
	void OnAudioPlayBegin();
	void OnAudioPlayData(BYTE* lpData, DWORD size);
	void OnAudioPlayStop();

	//void OnAudioData();
	static void __stdcall WorkThread(CAudio*pThis);
public:
	CAudio(CClient *pClient);

	static volatile unsigned int nInstance;
	void OnClose();
	void OnOpen();

	void OnEvent(UINT32 e, BYTE* lpData, UINT Size);
	~CAudio();
};

