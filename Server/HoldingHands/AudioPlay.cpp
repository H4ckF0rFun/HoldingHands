#include "stdafx.h"
#include "AudioPlay.h"


CAudioPlay::CAudioPlay():
	m_hWaveOut(NULL),
	m_Idx(0),
	m_IsWorking(FALSE)
{
	memset(m_hdrs, 0, sizeof(m_hdrs));
	memset(buffs, 0, sizeof(buffs));

	memset(&m_WaveFmt, 0, sizeof(m_WaveFmt));
	m_WaveFmt.wFormatTag = WAVE_FORMAT_PCM; // ACM will auto convert wave format
	m_WaveFmt.nChannels = 1;
	m_WaveFmt.nSamplesPerSec = 44100;
	m_WaveFmt.nAvgBytesPerSec = 44100 * 2;
	m_WaveFmt.nBlockAlign = 2;
	m_WaveFmt.wBitsPerSample = 16;
	m_WaveFmt.cbSize = 0;

}


CAudioPlay::~CAudioPlay()
{
	ClosePlayer();
}

BOOL CAudioPlay::InitPlayer(){
	ClosePlayer();

	MMRESULT mmResult = 0;
	//打开设备
	mmResult = waveOutOpen(&m_hWaveOut, WAVE_MAPPER,
		&m_WaveFmt, NULL, 0, CALLBACK_NULL);
	if (mmResult != MMSYSERR_NOERROR)
		return FALSE;

	memset(m_hdrs, 0, sizeof(m_hdrs));
	//
	for (int i = 0; i < 16; i++){
		m_hdrs[i].lpData = buffs[i];
		m_hdrs[i].dwBufferLength = LEN_PER_BUFF;
	}
	m_IsWorking = TRUE;
	return TRUE;
}

BOOL CAudioPlay::PlayBuffer(char*Buffer, DWORD dwLen){
	MMRESULT mmResult = 0;
	if (m_hWaveOut == NULL){
		return FALSE;
	}

	m_hdrs[m_Idx].dwBytesRecorded = 0;
	m_hdrs[m_Idx].dwFlags = 0;
	m_hdrs[m_Idx].dwLoops = 0;
	m_hdrs[m_Idx].dwUser = 0;
	m_hdrs[m_Idx].lpNext = 0;
	m_hdrs[m_Idx].reserved = 0;

	m_hdrs[m_Idx].dwBufferLength = dwLen;

	if (dwLen > LEN_PER_BUFF){
		return FALSE;
	}
	//拷贝buffer,
	memcpy(m_hdrs[m_Idx].lpData, Buffer, dwLen);

	mmResult = waveOutPrepareHeader(m_hWaveOut, &m_hdrs[m_Idx], sizeof(WAVEHDR));
	if (mmResult != MMSYSERR_NOERROR){
		return FALSE;
	}

	mmResult = waveOutWrite(m_hWaveOut, &m_hdrs[m_Idx], sizeof(WAVEHDR));
	if (mmResult != MMSYSERR_NOERROR){
		return FALSE;
	}

	m_Idx = (m_Idx + 1) % BUFF_COUNT;
	return TRUE;
}

void CAudioPlay::ClosePlayer(){
	//
	if (m_hWaveOut){
		waveOutPause(m_hWaveOut);
		waveOutClose(m_hWaveOut);
		m_hWaveOut = NULL;
	}
	//
	memset(m_hdrs, 0, sizeof(m_hdrs));
	m_Idx = 0;
	m_IsWorking = FALSE;
}