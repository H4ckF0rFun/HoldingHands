#pragma once


#include <Windows.h>

//#include "DxgiScreenGrab.h"
#include <stdint.h>

extern "C"{
#include "libyuv.h"
#include "x264.h"
}

class CDesktopGrab
{
private:
	//BOOL	m_bUseDxgiGrab;
	//DxgiScreenGrab m_DxgiGrab;

	DWORD	m_dwWidth;				//·Ö±æÂÊ
	DWORD	m_dwHeight;
	//gdi grab
	DWORD	m_dwScreenWidth;		//..
	DWORD	m_dwScreenHeight;

	HDC		m_hDC;
	HDC		m_hMemDC;
	HBITMAP	m_hBmp;
	BITMAP	m_Bmp;
	void*	m_Buffer;
	//
	DWORD	m_dwStride;
	DWORD	m_dwBpp;		//bits per pix.
	x264_t*	m_pEncoder;		//±àÂëÆ÷ÊµÀý

	x264_picture_t *m_pPicIn;
	x264_picture_t *m_pPicOut;

	
	void	DrawMouse();
public:
	BOOL	GrabInit(DWORD dwFps,DWORD Quality);
	void	GrabTerm();
	void	GetDesktopSize(DWORD *pWidth, DWORD*pHeight);
	BOOL	GetFrame(BYTE**lppbuffer,DWORD*pSize,DWORD dwCaptureFlags);

	BOOL	GetBmpFile(BYTE**lppBuffer, DWORD*lpSize);
	CDesktopGrab();
	~CDesktopGrab();
};

