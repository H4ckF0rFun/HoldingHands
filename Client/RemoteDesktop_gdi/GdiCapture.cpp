#include "GdiCapture.h"
#include "dbg.h"

CGdiCapture::CGdiCapture()
{
	m_currentOutputIdx = -1;
	m_nMonitors = 0;
	m_hBmp = NULL;
	m_hMemDC = NULL; 
	
	m_lpBits = NULL;

	memset(&m_Bmp, 0, sizeof(m_Bmp)); 
	memset(m_MonitorRect, 0, sizeof(m_MonitorRect));
}

CGdiCapture::~CGdiCapture()
{
	Cleanup();
}

void CGdiCapture::Cleanup()
{
	if (m_hMemDC)
	{
		DeleteDC(m_hMemDC);
		m_hMemDC = NULL;
	}

	if (m_hBmp)
	{
		DeleteObject(m_hBmp);
		m_hBmp = NULL;
		m_lpBits = NULL;
		memset(&m_Bmp, 0, sizeof(BITMAP));
	}

	m_currentOutputIdx = -1;
}



BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MONITORINFOEX mi;
	LPVOID * ArgList   = (LPVOID*)dwData;
	int  *   n         = (int*)ArgList[0];
	RECT *   lpMonitor = (RECT *)ArgList[1];
	DWORD iModeNum = 0;
	DEVMODE dmi;

	mi.cbSize = sizeof(MONITORINFOEX);
	
	GetMonitorInfo(hMonitor, &mi);

	ZeroMemory(&dmi, sizeof(dmi));
	dmi.dmSize = sizeof(dmi);

	if (!EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, &dmi)){
		return FALSE;
	}

	//DWORD  dmPelsWidth;
	//DWORD  dmPelsHeight;

	if (*n < GDICAPTURE_MAX_MONITORS)
	{
		lpMonitor[*n].left = dmi.dmPosition.x;
		lpMonitor[*n].top = dmi.dmPosition.y;

		lpMonitor[*n].right = dmi.dmPosition.x + dmi.dmPelsWidth;
		lpMonitor[*n].bottom = dmi.dmPosition.y + dmi.dmPelsHeight;

		++(*n);
	}

	return TRUE;
}

#include <shellscalingapi.h>
typedef HRESULT (*typeSetProcessDpiAwareness)( PROCESS_DPI_AWARENESS value);

int  CGdiCapture::InitGdiDevice()
{
	LPVOID ArgList[2];
	
	ArgList[0] = &m_nMonitors;
	ArgList[1] = &m_MonitorRect;


	if (!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)ArgList))
		return -1;

	return 0;
}

int CGdiCapture::GetAllMonitor(RECT * lpMonitors)
{
	for (int i = 0; i < m_nMonitors; i++)
	{
		lpMonitors->left = 0;
		lpMonitors->top = 0;
		lpMonitors->right = m_MonitorRect[i].right - m_MonitorRect[i].left;
		lpMonitors->bottom = m_MonitorRect[i].bottom - m_MonitorRect[i].top;
		lpMonitors++;
	}
	return m_nMonitors;
}

int  CGdiCapture::InitCapture(int output_idx)
{
	HDC hDC;
	int err = 0;
	int Width, Height;
	BITMAPINFO bmi = { 0 };

	err = -1;
	if (output_idx > m_nMonitors){
		goto failed;
	}

	err = -2;
	hDC = GetDC(NULL);
	if (hDC == NULL){
		goto failed;
	}

	err = -3;
	m_hMemDC = CreateCompatibleDC(hDC);
	if (m_hMemDC == NULL){
		goto failed;
	}

	Width = m_MonitorRect[output_idx].right - m_MonitorRect[output_idx].left;
	Height = m_MonitorRect[output_idx].bottom - m_MonitorRect[output_idx].top;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = Width;
	bmi.bmiHeader.biHeight = Height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	
	bmi.bmiHeader.biCompression = BI_RGB;

	err = -4;
	m_hBmp = CreateDIBSection(m_hMemDC, &bmi, DIB_RGB_COLORS, &m_lpBits, 0, 0);
	if (m_hBmp == NULL){
		goto failed;
	}

	err = -5;
	if (!SelectObject(m_hMemDC, m_hBmp)){
		goto failed;
	}

	GetObject(m_hBmp, sizeof(BITMAP), &m_Bmp);

	ReleaseDC(NULL, hDC);
	hDC = NULL;
	m_currentOutputIdx = output_idx;
	return 0;

failed:
	if (hDC){
		ReleaseDC(NULL, hDC);
		hDC = NULL;
	}
	Cleanup();
	return err;
}



int CGdiCapture::GetDesktopFrame(uint8_t ** lpRgb, uint32_t *lpStride, uint32_t* lpSize, DWORD flag )
{
	int err = 0;
	HDC hDC = GetDC(NULL);
	int Width, Height;
	
	err = -1;
	if (hDC == NULL){
		goto failed;
	}

	Width = m_MonitorRect[m_currentOutputIdx].right - m_MonitorRect[m_currentOutputIdx].left;
	Height = m_MonitorRect[m_currentOutputIdx].bottom - m_MonitorRect[m_currentOutputIdx].top;

	err = -2;
	if (!BitBlt(
		m_hMemDC,
		0,
		0,
		Width,
		Height,
		hDC,
		m_MonitorRect[m_currentOutputIdx].left,
		m_MonitorRect[m_currentOutputIdx].top,
		SRCCOPY | flag)){
		goto failed;
	}

	*lpRgb = (uint8_t*)m_lpBits;
	*lpStride = m_Bmp.bmWidthBytes;
	*lpSize = m_Bmp.bmHeight * m_Bmp.bmWidthBytes;

	ReleaseDC(NULL, hDC);
	hDC = NULL;

	return 0;

failed:
	if (hDC)
	{
		ReleaseDC(NULL, hDC);
		hDC = NULL;
	}
	return err;
}

