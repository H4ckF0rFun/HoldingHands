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


//Norify Message..
#define WM_REMOTE_DESKTOP_ERROR					(WM_USER + 69)
#define WM_REMOTE_DESKTOP_DRAW					(WM_USER + 70)
#define WM_REMOTE_DESKTOP_SIZE					(WM_USER + 71)
#define WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT	(WM_USER + 72)
#define WM_REMOTE_DESKTOP_SCREENSHOT			(WM_USER + 73)
#define WM_REMOTE_DESKTOP_GET_DRAW_WND			(WM_USER + 74)

//

#include "EventHandler.h"

class CRemoteDesktopSrv :
	public CEventHandler
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

	BOOL RemoteDesktopSrvInit(DWORD dwWidth,DWORD dwHeight);
	void RemoteDesktopSrvTerm();
	
	//Event
	void OnDeskSize(char*DeskSize);
	void OnError(char* szError);
	void OnFrame(DWORD dwRead,BYTE *Buffer);
	
	void OnBmpFile(BYTE * Buffer, DWORD dwSize);
	void OnSetClipboardText(TCHAR*Text);
	void NextFrame();
public:
	void StartRDP(DWORD dwMaxFps, DWORD dwQuality);

	
	void ScreenShot();
	void Control(CtrlParam*pParam);
	void SetClipboardText(TCHAR *szText);
	void SetCaptureFlag(DWORD dwFlag);

	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数


	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

	CRemoteDesktopSrv(CClient*pClient);
	~CRemoteDesktopSrv();
};

