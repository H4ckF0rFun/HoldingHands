//#pragma once
//
//
//#include <Windows.h>
//#include <stdint.h>
//#include "DxgiCapture.h"
//
//extern "C"{
//#include "libyuv.h"
//#include "x264.h"
//}
//
//class CDesktopGrab
//{
//private:
//	
//
//	//
//	DWORD	m_dwStride;
//	DWORD	m_dwBpp;		//bits per pix.
//	
//	//
//	x264_t*	m_pEncoder;		//±àÂëÆ÷ÊµÀý
//	x264_picture_t *m_pPicIn;
//	x264_picture_t *m_pPicOut;
//
//	CDxgiCapture m_dxgiCapture;
//
//
//public:
//	BOOL	GrabInit(DWORD dwFps,DWORD Quality);
//	void	GrabTerm();
//	void	GetDesktopSize(DWORD *pWidth, DWORD*pHeight);
//	BOOL	GetFrame(BYTE**lppbuffer,DWORD*pSize,DWORD dwCaptureFlags);
//
//	//BOOL	GetBmpFile(BYTE**lppBuffer, DWORD*lpSize);
//	CDesktopGrab();
//	~CDesktopGrab();
//};
//
