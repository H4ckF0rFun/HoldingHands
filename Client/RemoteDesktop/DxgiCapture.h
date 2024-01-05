#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdint.h>

class CDxgiCapture
{
private:

public:
	CDxgiCapture();
	~CDxgiCapture();

public:
	int  InitD3D11Device();
	int  InitDuplication(int output_id);
	int  InitTexture();
	int  GetDesktopFrame(uint8_t ** lpRgb, uint32_t *lpStride, uint32_t* lpSize);
	int  GetAllMonitor(RECT * lpMonitors);
	void GetCurrentMonitorSize(int *lpWidth, int *lpHeight){
		*lpWidth = m_MonitorRect.right - m_MonitorRect.left;
		*lpHeight = m_MonitorRect.bottom - m_MonitorRect.top;
	}

	void GetCurrentMonitorRect(RECT * lpRect){
		*lpRect = m_MonitorRect;
	}

	void Cleanup();

private:
	RECT				    m_MonitorRect;

	int						m_currentOutputIdx;
	ID3D11Device *		    m_pDevice;
	ID3D11DeviceContext *   m_pDeviceContext;


	//current monitor..
	IDXGIOutputDuplication * m_pDuplication;
	ID3D11Texture2D*		 m_Texture;
	D3D11_MAPPED_SUBRESOURCE m_Mapped_resource;

};

