#pragma once
#include "EventHandler.h"
#include "AudioGrab.h"
#include "AudioPlay.h"
#include "audio_common.h"


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

	//≤•∑≈…˘“Ù.
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

