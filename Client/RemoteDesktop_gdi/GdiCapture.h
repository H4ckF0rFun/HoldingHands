#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdint.h>

#define GDICAPTURE_MAX_MONITORS 16

class CGdiCapture
{
private:

public:
	CGdiCapture();
	~CGdiCapture();

public:

	int  InitGdiDevice();
	int  InitCapture(int output_idx);

	int  GetAllMonitor(RECT * lpMonitors);
	int  GetDesktopFrame(uint8_t ** lpRgb, uint32_t *lpStride, uint32_t* lpSize,DWORD flag = 0);
	
	void GetCurrentMonitorSize(int *lpWidth, int *lpHeight){
		*lpWidth = m_MonitorRect[m_currentOutputIdx].right - m_MonitorRect[m_currentOutputIdx].left;
		*lpHeight = m_MonitorRect[m_currentOutputIdx].bottom - m_MonitorRect[m_currentOutputIdx].top;
	}

	void GetCurrentMonitorRect(RECT * lpRect){
		*lpRect = m_MonitorRect[m_currentOutputIdx];
	}

	void Cleanup();

private:

	RECT	m_MonitorRect[GDICAPTURE_MAX_MONITORS];
	int     m_nMonitors;

	int		m_currentOutputIdx;
	HDC		m_hMemDC;
	HBITMAP	m_hBmp;
	BITMAP	m_Bmp;

	void*	m_lpBits;
};

