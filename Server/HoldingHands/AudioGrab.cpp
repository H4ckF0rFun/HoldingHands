#include "stdafx.h"
#include "AudioGrab.h"


#pragma comment(lib,"Winmm.lib")
CAudioGrab::CAudioGrab()
{
	m_hEvent = NULL;
	m_hMutex = NULL;
	
	m_hWaveIn = NULL;
	m_dwBufSize = 0;
	m_hWorkThread = NULL;
	m_dwThreadID = NULL;
	m_Idx = 0;

	m_IsWorking = FALSE;

	memset(&m_Wavefmt, 0, sizeof(m_Wavefmt));
	//带宽足够了,就不压缩了
	m_Wavefmt.wFormatTag = WAVE_FORMAT_PCM; // ACM will auto convert wave format
	m_Wavefmt.nChannels = 1;
	m_Wavefmt.nSamplesPerSec = 44100;
	m_Wavefmt.nAvgBytesPerSec = 44100 * 2;
	m_Wavefmt.nBlockAlign = 2;
	m_Wavefmt.wBitsPerSample = 16;
	m_Wavefmt.cbSize = 0;
}


CAudioGrab::~CAudioGrab()
{
	CloseGrabber();
}



BOOL CAudioGrab::InitGrabber()
{
	MMRESULT mmResult = 0;
	int i = 0;
	CloseGrabber();
	
	m_hWorkThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WorkThread, this, 0, &m_dwThreadID);
	if (!m_hWorkThread){
		goto Failed;
	}
		
	mmResult = waveInOpen(&m_hWaveIn, WAVE_MAPPER, &m_Wavefmt, 
		(DWORD_PTR)m_dwThreadID, (DWORD_PTR)this, CALLBACK_THREAD);
	//
	if (mmResult != MMSYSERR_NOERROR){
		goto Failed;
	}

	for (i = 0; i < BUFF_COUNT; i++){

		m_hdrs[i].dwBufferLength = LEN_PER_BUFF;
		m_hdrs[i].lpData = m_Buffers[i];
		memset(m_Buffers, 0, LEN_PER_BUFF);

		mmResult = waveInPrepareHeader(m_hWaveIn, &m_hdrs[i], sizeof(WAVEHDR));
		if (mmResult != MMSYSERR_NOERROR){
			goto Failed;
		}
			
		mmResult = waveInAddBuffer(m_hWaveIn, &m_hdrs[i], sizeof(WAVEHDR));
		if (mmResult != MMSYSERR_NOERROR){
			goto Failed;
		}
	}
	m_hMutex = CreateEvent(NULL, FALSE, TRUE, NULL);
	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	waveInStart(m_hWaveIn);
	m_IsWorking = TRUE;
	return TRUE;

Failed:
	CloseGrabber();
	return FALSE;
}

void CAudioGrab::CloseGrabber()
{
	//停止播放..
	if (m_hWaveIn)
		waveInStop(m_hWaveIn);

	//等待线程退出
	if (m_hWorkThread && m_dwThreadID){
		PostThreadMessage(m_dwThreadID, WM_QUIT, 0, 0);
		WaitForSingleObject(m_hWorkThread, INFINITE);
		CloseHandle(m_hWorkThread);
	}

	m_hWorkThread = 0;
	m_dwThreadID = 0;
	
	//关闭设备
	if (m_hWaveIn){
		waveInClose(m_hWaveIn);
		m_hWaveIn = NULL;
	}
	//
	if (m_hEvent){
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	if (m_hMutex){
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}
	//清理缓存
	while (!m_buffer_list.empty()){
		auto & buff = m_buffer_list.front();
		if (buff.first){
			delete[]buff.first;
		}
		m_buffer_list.pop_front();
	}
	//
	m_dwBufSize = 0;
	m_Idx = 0;
	memset(m_hdrs, 0, sizeof(m_hdrs));
	m_IsWorking = FALSE;
}

BOOL CAudioGrab::GetBuffer(void**ppBuffer, DWORD*pBufLen)
{
	//printf("Get Buffer()\n");
	char*Buffer = NULL;
	DWORD dwLen = NULL;
	RemoveHead(&Buffer, &dwLen);
	if (Buffer == NULL){
		return FALSE;
	}
	*ppBuffer = Buffer;
	*pBufLen = dwLen;
	return TRUE;
}

void __stdcall CAudioGrab::WorkThread(CAudioGrab*pThis)
{
	//callback function会造成死锁.改为线程.
	MSG msg;
	WAVEHDR*pHdr = 0;
	MMRESULT mmResult = 0;
	while (GetMessage(&msg, 0, 0, 0))
	{
		switch (msg.message)
		{
		case MM_WIM_DATA:
			pHdr = &pThis->m_hdrs[pThis->m_Idx];
			pThis->AddTail(pHdr->lpData, pHdr->dwBytesRecorded);

			pHdr->dwBytesRecorded = 0;
			pHdr->dwFlags = 0;
			pHdr->dwLoops = 0;
			pHdr->dwUser = 0;
			pHdr->lpNext = 0;
			pHdr->reserved = 0;

			waveInPrepareHeader(pThis->m_hWaveIn, pHdr, sizeof(WAVEHDR));
			if (!mmResult)
			{
				//printf("waveInPrepareHeader Ok!\n");
			}
			mmResult = waveInAddBuffer(pThis->m_hWaveIn, pHdr, sizeof(WAVEHDR));
			if (!mmResult)
			{
				//printf("waveInAddBuffer Ok!\n");
			}
			pThis->m_Idx = (pThis->m_Idx + 1) % BUFF_COUNT;
			break;
		default:
			break;
		}
	}
	//printf("Thread Exit\n");
}

void  CAudioGrab::RemoveHead(char**ppBuffer, DWORD*pLen)
{
	pair<char*, DWORD> buf(nullptr,0);
	*ppBuffer = nullptr;
	*pLen = 0;

	while (true){
		WaitForSingleObject(m_hMutex, INFINITE);

		if (m_buffer_list.size()){
			buf = m_buffer_list.front();
			m_buffer_list.pop_front();
		}

		if (!buf.first)
			ResetEvent(m_hEvent);
		else
			m_dwBufSize -= buf.second;

		SetEvent(m_hMutex);

		if (!buf.first){
			//printf("No Buf,Wait ForSingle Object\n");
			if (WAIT_TIMEOUT == WaitForSingleObject(m_hEvent, 3000)){
				//printf("Wait For Single Object Time Out\n");
				return;
			}
		}
		else
			break;
	}
	*ppBuffer = buf.first;
	*pLen = buf.second;

}
void  CAudioGrab::AddTail(char*Buffer, DWORD dwLen)
{
	if (m_dwBufSize > 128 * 1024 * 1024)
		return;

	pair<char*, DWORD> buf (new char[dwLen], dwLen);

	memcpy(buf.first, Buffer, dwLen);

	WaitForSingleObject(m_hMutex, INFINITE);

	m_dwBufSize += dwLen;

	m_buffer_list.push_back(buf);

	SetEvent(m_hEvent);
	SetEvent(m_hMutex);
}