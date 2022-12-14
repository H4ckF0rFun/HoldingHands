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


class CAudioDlg;
class CAudioSrv :
	public CMsgHandler
{

private:
	CAudioGrab m_AudioGrab;
	CAudioPlay m_AudioPlay;
	//send local voice.
	volatile unsigned int m_IsWorking;
	HANDLE	m_hWorkThread;

	CAudioDlg*	m_pDlg;

	//????????.
	void OnAudioPlayBegin();
	void OnAudioPlayData(char* buffer, DWORD size);
	void OnAudioPlayStop();

public:
	void OnOpen();
	void OnClose();

	void OnAudioError(TCHAR*szError);

	void StartSendLocalVoice();
	void StopSendLocalVoice();

	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) { }; 

	static void __stdcall CAudioSrv::SendThread(CAudioSrv*pThis);

	CAudioSrv(CManager*pManager);
	~CAudioSrv();
};

