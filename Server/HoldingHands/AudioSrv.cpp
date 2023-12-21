#include "stdafx.h"
#include "AudioSrv.h"
#pragma comment(lib,"Winmm.lib")


CAudioSrv::CAudioSrv(CClient*pClient) :
CEventHandler(pClient,AUDIO)
{
	m_IsWorking = FALSE;
	m_hWorkThread = NULL;
}


CAudioSrv::~CAudioSrv()
{
	dbg_log("CAudioSrv::~CAudioSrv()");
}

void CAudioSrv::OnOpen()
{
	//开始
	Send(AUDIO_BEGIN, 0, 0);
}

void CAudioSrv::OnClose()
{
	StopSendLocalVoice();
	OnAudioPlayStop();

	Notify(WM_AUDIO_ERROR, (WPARAM)TEXT("Connection close..."));
}



void CAudioSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case AUDIO_PLAY_BEGIN:
		OnAudioPlayBegin();
		break;
	case AUDIO_PLAY_DATA:
		OnAudioPlayData((char*)lpData, Size);
		break;
	case AUDIO_PLAY_STOP:
		OnAudioPlayStop();
		break;
	case AUDIO_ERROR:
		OnAudioError((char*)lpData);
		break;
	default:
		break;
	}
}


void CAudioSrv::OnAudioError(char*szError)
{
	Notify(WM_AUDIO_ERROR, (WPARAM)szError, 0);
}

void CAudioSrv::OnAudioPlayBegin()
{
	OnAudioPlayStop();
	m_AudioPlay.InitPlayer();
}

void CAudioSrv::OnAudioPlayData(char* buffer, DWORD size)
{
	if (!m_AudioPlay.IsWorking())
	{
		Notify(
			WM_AUDIO_ERROR,
			(WPARAM)(TEXT("AudioPlayer Is not working."))
		);
		return;
	}

	if (!m_AudioPlay.PlayBuffer(buffer, size))
	{
		Notify(
			WM_AUDIO_ERROR,
			(WPARAM)(TEXT("AudioPlay - PlayBuffer Failed!"))
		);
		Close();			//断开连接
		return;
	}
}

void CAudioSrv::OnAudioPlayStop(){
	m_AudioPlay.ClosePlayer();
}

//这里是发送本地语音到远程.
void CAudioSrv::StartSendLocalVoice(){
	StopSendLocalVoice();

	InterlockedExchange(&m_IsWorking, TRUE);
	if (m_AudioGrab.InitGrabber())
	{
		Send(AUDIO_PLAY_BEGIN, 0, 0);		//通知对方打开播放器...
		m_hWorkThread = CreateThread(
			0, 
			0, 
			(LPTHREAD_START_ROUTINE)SendThread,
			this,
			0, 
			0);
	}
	else
	{
		InterlockedExchange(&m_IsWorking, FALSE);
		Notify(
			WM_AUDIO_ERROR,
			(WPARAM)TEXT("Local Audio Grab Init Failed")
		);
	}
}

void CAudioSrv::StopSendLocalVoice(){
	InterlockedExchange(&m_IsWorking, FALSE);

	if (m_hWorkThread)
	{
		WaitForSingleObject(m_hWorkThread, INFINITE);
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	m_AudioGrab.CloseGrabber();
	Send(AUDIO_PLAY_STOP, 0, 0);
}

void __stdcall CAudioSrv::SendThread(CAudioSrv*pThis)
{
	TCHAR szError[] = TEXT("Get Audio Buffer Error!");
	while (InterlockedExchange(&pThis->m_IsWorking, TRUE))
	{
		char*Buffer = NULL;
		DWORD dwLen = NULL;
		//获取buffer发送
		if (pThis->m_AudioGrab.GetBuffer((void**)&Buffer, &dwLen))
		{
			pThis->Send(AUDIO_PLAY_DATA, Buffer, dwLen);
			free(Buffer);
		}
	}
	InterlockedExchange(&pThis->m_IsWorking, FALSE);
}