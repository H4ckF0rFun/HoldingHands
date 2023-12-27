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

//notify 
#define WM_AUDIO_ERROR (WM_USER + 137)

class CAudioSrv :
	public CEventHandler
{

private:
	CAudioGrab m_AudioGrab;
	CAudioPlay m_AudioPlay;
	//send local voice.
	volatile unsigned int m_IsWorking;
	HANDLE	m_hWorkThread;

	//²¥·ÅÉùÒô.
	void OnAudioPlayBegin();
	void OnAudioPlayData(char* buffer, DWORD size);
	void OnAudioPlayStop();

public:
	

	void OnAudioError(char*szError);

	void StartSendLocalVoice();
	void StopSendLocalVoice();

	void OnOpen();
	void OnClose();
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);


	static void __stdcall CAudioSrv::SendThread(CAudioSrv*pThis);

	CAudioSrv(CClient*pClient);
	~CAudioSrv();
};

