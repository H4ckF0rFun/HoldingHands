#pragma once
#include "EventHandler.h"
#include "camera_common.h"
#include "resource.h"
#include <string>

using std::string;

extern"C"
{
	#include <libavcodec\avcodec.h>
	#include <libavutil\avutil.h>
	#include <libyuv.h>
}

//Notify Message
#define WM_CAMERA_DEVICELIST	(WM_USER + 352)
#define WM_CAMERA_ERROR			(WM_USER + 353)
#define WM_CAMERA_FRAME			(WM_USER + 354)
#define WM_CAMERA_VIDEOSIZE		(WM_USER + 355)
#define WM_CAMERA_STOP_OK		(WM_USER + 357)
#define WM_CAMERA_GET_DRAW_WND	(WM_USER + 358)


class CCameraSrv :
	public CEventHandler
{
private:
	AVCodec*			m_pCodec;
	AVCodecContext*		m_pCodecContext;
	AVPacket			m_AVPacket;
	AVFrame				m_AVFrame;

	HBITMAP				m_hBmp;
	BITMAP				m_Bmp;
	HDC					m_hMemDC;
	void*				m_Buffer;

	HANDLE				m_hMutex;

	int		CameraInit(int width, int height);
	void	CameraTerm();
public:
	void OnOpen();
	void OnClose();
	
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);

	void OnFrame(char*Buffer, DWORD dwLen);
	void OnVideoSize(int code,string&err,int width,int height);
	void OnStopOk();
	void OnDeviceList(char*DeviceList);

	void Start(const char* device, int Width, int Height);
	void Stop();
	void OnError(char*szError);


	char * GetBmpFile(DWORD * lpDataSize);

	CCameraSrv(CClient*pClient);
	~CCameraSrv();
};

