#pragma once
#include "MsgHandler.h"
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
	public CMsgHandler
{
	CAudioGrab m_AudioGrab;
	CAudioPlay m_AudioPlay;

	HANDLE	   m_hWorkThread;
	volatile long m_IsWorking;

	void OnAudioBegin();
	void OnAudioStop();
	
	//????????.
	void OnAudioPlayBegin();
	void OnAudioPlayData(char* buffer, DWORD size);
	void OnAudioPlayStop();

	//void OnAudioData();
	static void __stdcall WorkThread(CAudio*pThis);
public:
	CAudio(CManager*pManager);

	void OnClose();
	void OnOpen();

	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){}

	~CAudio();
};

