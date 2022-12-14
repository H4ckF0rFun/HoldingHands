#pragma once
extern "C"
{
#include <libavcodec\avcodec.h>
#include <libavutil\avutil.h>
#include <libyuv.h>
}

#define REMOTEDESKTOP	('R'|('D'<<8)|('T'<<16)|('P'<<24))


#define REMOTEDESKTOP_INIT_RDP		(0xaa01)
#define REMOTEDESKTOP_DESKSIZE		(0xaa02)

#define REMOTEDESKTOP_NEXT_FRAME	(0xaaa1)
#define REMOTEDESKTOP_FRAME			(0xaaa2)
#define REMOTEDESKTOP_ERROR			(0xaaa3)

#define REMOTEDESKTOP_CTRL			(0xaaa4)

#define REMOTEDESKTOP_SETFLAG		(0xaaa6)

//设置剪切板数据.
#define REMOTEDESKTOP_SET_CLIPBOARDTEXT	(0xaaa7)
#define REMOTEDESKTOP_GET_BMP_FILE		(0xaaa8)
#define REMOTEDESKTOP_BMP_FILE			(0xaaa9)


#define REMOTEDESKTOP_FLAG_CAPTURE_MOUSE		(0x1)
#define REMOTEDESKTOP_FLAG_CAPTURE_TRANSPARENT	(0x2)


#define QUALITY_LOW		0
#define QUALITY_HIGH	2

#include "MsgHandler.h"

class CRemoteDesktopWnd;



class CRemoteDesktopSrv :
	public CMsgHandler
{
public:
	struct CtrlParam
	{
		DWORD dwType;
		union
		{
			DWORD dwCoor;
			DWORD VkCode;
		}Param;
		DWORD dwExtraData;
	};
private:
	AVCodec*			m_pCodec;
	AVCodecContext*		m_pCodecContext;
	AVPacket			m_AVPacket;
	AVFrame				m_AVFrame;

	HBITMAP				m_hBmp;
	BITMAP				m_Bmp;
	HDC					m_hMemDC;
	void*				m_Buffer;				//不需要释放。。。。

	HANDLE				m_hMutex;

	CRemoteDesktopWnd*	m_pWnd;

	BOOL RemoteDesktopSrvInit(DWORD dwWidth,DWORD dwHeight);
	void RemoteDesktopSrvTerm();
	
	//Event
	void OnDeskSize(char*DeskSize);
	void OnError(TCHAR* szError);
	void OnFrame(DWORD dwRead,char*Buffer);
	
	void OnBmpFile(char * Buffer, DWORD dwSize);
	void OnSetClipboardText(char*Text);
	void NextFrame();
public:
	void StartRDP(DWORD dwMaxFps, DWORD dwQuality);

	
	void ScreenShot();
	void Control(CtrlParam*pParam);
	void SetClipboardText(char*szText);
	void SetCaptureFlag(DWORD dwFlag);

	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数


	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer);

	CRemoteDesktopSrv(CManager*pManager);
	~CRemoteDesktopSrv();
};

