#include "stdafx.h"
#include "RemoteDesktopSrv.h"
#include "utils.h"
#include "dbg.h"

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"yuv.lib")

//#pragma comment(lib,"avformat.lib")


#define MAX_CURSOR_TYPE	16

LPCTSTR	CursorResArray[MAX_CURSOR_TYPE] =
{
	IDC_APPSTARTING,
	IDC_ARROW,
	IDC_CROSS,
	IDC_HAND,
	IDC_HELP,
	IDC_IBEAM,
	IDC_ICON,
	IDC_NO,
	IDC_SIZE,
	IDC_SIZEALL,
	IDC_SIZENESW,
	IDC_SIZENS,
	IDC_SIZENWSE,
	IDC_SIZEWE,
	IDC_UPARROW,
	IDC_WAIT
};

CRemoteDesktopSrv::CRemoteDesktopSrv(CClient*pClient) :
CEventHandler(pClient, REMOTEDESKTOP),
	m_pCodec(NULL),
	m_pCodecContext(NULL),
	m_hBmp(NULL),
	m_hMemDC(NULL)
{
	memset(&m_AVPacket, 0, sizeof(AVPacket));
	memset(&m_AVFrame, 0, sizeof(AVFrame));
	memset(&m_Bmp, 0, sizeof(m_Bmp));

	m_hMutex = CreateEvent(0, TRUE, TRUE, NULL);
}


CRemoteDesktopSrv::~CRemoteDesktopSrv()
{
	CloseHandle(m_hMutex);
	m_hMutex = NULL;
}


void CRemoteDesktopSrv::OnOpen()
{
}


void CRemoteDesktopSrv::OnClose()
{
	RemoteDesktopSrvTerm();
	Notify(WM_REMOTE_DESKTOP_ERROR, (WPARAM)TEXT("Connection close..."));
}

BOOL CRemoteDesktopSrv::RemoteDesktopSrvInit(DWORD dwWidth, DWORD dwHeight)
{
	BITMAPINFO bmi = { 0 };
	HWND       hDrawWnd = NULL;
	HDC		   hDC = NULL;
	
	//创建内存dc
	hDrawWnd = (HWND)Notify(WM_REMOTE_DESKTOP_GET_DRAW_WND);
	hDC = GetDC(hDrawWnd);
	m_hMemDC = CreateCompatibleDC(hDC);
	ReleaseDC(hDrawWnd, hDC);

	if (m_hMemDC == NULL)
		goto error;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = dwWidth;
	bmi.bmiHeader.biHeight = dwHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;				//4字节貌似解码快
	bmi.bmiHeader.biCompression = BI_RGB;
	
	//创建DIBSection
	m_hBmp = CreateDIBSection(m_hMemDC, &bmi, DIB_RGB_COLORS, &m_Buffer, 0, 0);
	
	if (m_hBmp == NULL || !SelectObject(m_hMemDC, m_hBmp))
		goto error;
	//
	GetObject(m_hBmp, sizeof(BITMAP), &m_Bmp);
	
	//创建解码器.
	m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (m_pCodec == NULL)
		goto error;
	//
	m_pCodecContext = avcodec_alloc_context3(m_pCodec);
	if (m_pCodecContext == NULL)
		goto error;
	//
	if (0 != avcodec_open2(m_pCodecContext, m_pCodec, 0))
		goto error;

	return TRUE;

error:
	return FALSE;
}

void CRemoteDesktopSrv::RemoteDesktopSrvTerm()
{
	if (m_hMemDC)
	{
		DeleteDC(m_hMemDC);
		m_hMemDC = NULL;
	}
	
	if (m_hBmp)
	{
		DeleteObject(m_hBmp);
		memset(&m_Bmp, 0, sizeof(m_Bmp));
		m_hBmp = NULL;
	}

	if (m_pCodecContext)
	{
		avcodec_free_context(&m_pCodecContext);
		m_pCodecContext = 0;
	}
	m_pCodec = 0;
	//AVFrame需要清除
	av_frame_unref(&m_AVFrame);
	//
	memset(&m_AVPacket, 0, sizeof(m_AVPacket));
	memset(&m_AVFrame, 0, sizeof(m_AVFrame));

}


void CRemoteDesktopSrv::NextFrame()
{
	Send(REMOTEDESKTOP_NEXT_FRAME,NULL, 0);
}

void CRemoteDesktopSrv::ScreenShot()
{
	Send(REMOTEDESKTOP_GET_BMP_FILE, 0, 0);
}

void CRemoteDesktopSrv::OnBmpFile(BYTE * lpData, DWORD dwSize)
{
	Notify(WM_REMOTE_DESKTOP_SCREENSHOT, (WPARAM)lpData, dwSize);
}


void CRemoteDesktopSrv::OnDeskSize(char*DeskSize)
{
	DWORD dwWidth =  ((DWORD*)(DeskSize))[0];
	DWORD dwHeight = ((DWORD*)(DeskSize))[1];

	/*************************************************************/
	RemoteDesktopSrvTerm();

	if (!RemoteDesktopSrvInit(dwWidth, dwHeight))
	{
		Notify(WM_REMOTE_DESKTOP_ERROR, (WPARAM)TEXT("RemoteDesktopSrvInit Error"));
		Close();
		return;
	}

	NextFrame();
	Notify(WM_REMOTE_DESKTOP_SIZE, m_Bmp.bmWidth, m_Bmp.bmHeight);
}

void CRemoteDesktopSrv::OnError(char*szError)
{
	//桌面采集失败.
	Notify(WM_REMOTE_DESKTOP_ERROR, (WPARAM)szError, 0);
}


void CRemoteDesktopSrv::OnFrame(DWORD dwRead, BYTE *Buffer)
{
	DWORD CursorIconIdx = 1;
	HCURSOR hCursor     = NULL;
	int err		      	= 0;

	if (m_pCodecContext == NULL)
	{
		return;
	}

	if (Buffer[0] < MAX_CURSOR_TYPE)
	{
		CursorIconIdx = Buffer[0];
	}
	
	dwRead -= 1;
	Buffer += 1;

	//设置光标类型.
	hCursor = LoadCursor(NULL, CursorResArray[CursorIconIdx]);
	
	//解码数据.
	av_init_packet(&m_AVPacket);
	//
	m_AVPacket.data = (uint8_t*)Buffer;
	m_AVPacket.size = dwRead;

	////获取下一帧
	NextFrame();

	err = avcodec_send_packet(m_pCodecContext, &m_AVPacket);
	if (!err)
	{
		err = avcodec_receive_frame(m_pCodecContext, &m_AVFrame);
		if (err == AVERROR(EAGAIN)){
			dbg_log("avcodec_receive_frame error: EAGAIN\n");
			return;
		}

		//解码数据前会清除m_AVFrame的内容.
		if (!err)
		{
			LPVOID Image[2] = { 0 };
			//成功.
			//I420 ---> ARGB.
			WaitForSingleObject(m_hMutex,INFINITE);
			libyuv::I420ToARGB(
				m_AVFrame.data[0], m_AVFrame.linesize[0],
				m_AVFrame.data[1], m_AVFrame.linesize[1],
				m_AVFrame.data[2], m_AVFrame.linesize[2],
				(uint8_t*)m_Buffer, 
				m_Bmp.bmWidthBytes,
				m_Bmp.bmWidth, 
				m_Bmp.bmHeight);
			
			//显示到窗口上
			Image[0] = m_hMemDC;
			Image[1] = &m_Bmp;
			//
			Notify(WM_REMOTE_DESKTOP_DRAW, (WPARAM)Image, (LPARAM)hCursor);
			return;
		}
		dbg_log("avcodec_receive_frame failed with error: %d \n",err);
	}
	else{
		dbg_log("avcodec_send_packet failed with error: %d\n", err);
	}
	//失败
	Notify(WM_REMOTE_DESKTOP_ERROR, (WPARAM)("Decode Frame Error"));
	Close();
	return;
}


void CRemoteDesktopSrv::Control(CtrlParam*pParam)
{
	Send(REMOTEDESKTOP_CTRL, (char*)pParam, sizeof(CtrlParam));
}
/***************************************************************************
*	Event Handler
*
**************************************************************************/

void CRemoteDesktopSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case REMOTEDESKTOP_ERROR:
		OnError((char*)lpData);
		break;
	case REMOTEDESKTOP_DESKSIZE:
		OnDeskSize((char*)lpData);
		break;
	case REMOTEDESKTOP_FRAME:
		OnFrame(Size, lpData);
		break;
	case REMOTEDESKTOP_SET_CLIPBOARDTEXT:
		OnSetClipboardText((TCHAR*)lpData);
		break;
	case REMOTEDESKTOP_BMP_FILE:
		OnBmpFile(lpData, Size);
		break;
	default:
		break;
	}
}


void CRemoteDesktopSrv::SetClipboardText(TCHAR*szText)
{
	//设置剪切板内容
	Send(REMOTEDESKTOP_SET_CLIPBOARDTEXT, szText, sizeof(TCHAR) * (lstrlen(szText) + 1));
}

void CRemoteDesktopSrv::OnSetClipboardText(TCHAR*Text)
{
	Notify(WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT, (WPARAM)Text);
}

void CRemoteDesktopSrv::SetCaptureFlag(DWORD dwFlag)
{
	Send(REMOTEDESKTOP_SETFLAG, (char*)&dwFlag, sizeof(dwFlag));
}

void CRemoteDesktopSrv::StartRDP(DWORD dwMaxFps, DWORD dwQuality)
{
	vec bufs[2];
	bufs[0].lpData = &dwMaxFps;
	bufs[0].Size = sizeof(dwMaxFps);

	bufs[1].lpData = &dwQuality;
	bufs[1].Size = sizeof(dwQuality);

	Send(REMOTEDESKTOP_INIT_RDP,bufs , 2);
}