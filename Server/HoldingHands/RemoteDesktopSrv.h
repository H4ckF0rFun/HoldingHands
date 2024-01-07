#pragma once
extern "C"
{
#include <libavcodec\avcodec.h>
#include <libavutil\avutil.h>
#include <libyuv.h>
}

#include "EventHandler.h"
#include "rd_common.h"


//Norify Message..
#define WM_REMOTE_DESKTOP_ERROR					(WM_USER + 69)
#define WM_REMOTE_DESKTOP_DRAW					(WM_USER + 70)
#define WM_REMOTE_DESKTOP_SIZE					(WM_USER + 71)
#define WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT	(WM_USER + 72)
#define WM_REMOTE_DESKTOP_SCREENSHOT			(WM_USER + 73)
#define WM_REMOTE_DESKTOP_GET_DRAW_WND			(WM_USER + 74)
#define WM_REMOTE_DESKTOP_MONITORS				(WM_USER + 76)
//



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
	LPVOID				m_lpBits;				//不需要释放。。。。

	HDC					m_hMemDC;
	DWORD				m_flag;


	BOOL RemoteDesktopSrvInit(DWORD dwWidth,DWORD dwHeight);
	void RemoteDesktopSrvTerm();
	
	//Event
	void OnDeskSize(char*DeskSize);
	void OnError(char* szError);
	void OnFrame(DWORD dwRead,BYTE *Buffer);
	
	//void OnBmpFile(BYTE * Buffer, DWORD dwSize);

	void OnSetClipboardText(TCHAR*Text);
	void NextFrame();
	void OnMonitorsInfo(RECT * lpMonitors, int n);

public:
	void StartCapture(int id, DWORD dwMaxFps, DWORD dwQuality);
	void ScreenShot(CONST TCHAR * szFileName);
	void Control(CtrlParam*pParam);
	void SetClipboardText(TCHAR *szText);
	void SetFlag(DWORD dwFlag);
	DWORD GetFlag() { return m_flag;  };

	void OnClose();				//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数

	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

	CRemoteDesktopSrv(CClient*pClient);
	~CRemoteDesktopSrv();
};

