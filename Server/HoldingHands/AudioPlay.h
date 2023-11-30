#pragma once

#include <windef.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <list> 


#define BUFF_COUNT 16
#define LEN_PER_BUFF 0x10000

class CAudioPlay
{
private:
	WAVEHDR	 m_hdrs[BUFF_COUNT];
	char	 buffs[BUFF_COUNT][LEN_PER_BUFF];
	HWAVEOUT m_hWaveOut;
	WAVEFORMATEX m_WaveFmt;	//∏Ò Ω
	DWORD	 m_Idx;
	BOOL	 m_IsWorking;

public:
	BOOL IsWorking(){ return m_IsWorking; };
	BOOL InitPlayer();
	BOOL PlayBuffer(char*Buffer, DWORD dwLen);
	void ClosePlayer();
	CAudioPlay();
	~CAudioPlay();
};

