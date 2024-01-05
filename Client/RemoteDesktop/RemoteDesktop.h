#pragma once
#include "EventHandler.h"
#include "rd_common.h"
#include "module.h"
#include "DxgiCapture.h"
#include "X264Encoder.h"

class CRemoteDesktop :
	public CEventHandler
{
private:
	typedef struct
	{
		DWORD dwType;
		union
		{
			DWORD dwCoor;
			DWORD VkCode;
		}Param;
		DWORD dwExtraData;
	}CtrlParam;

	typedef struct MyData
	{
		CRemoteDesktop* m_pThis;
		HWND			m_hNextViewer;
		TCHAR*			m_SetClipbdText;
	}MyData;

private:
	Module *	  m_owner;

	volatile long m_FrameReqCnt;
	volatile long m_TimerPerFrame;
	HANDLE m_hTimerQueue;
	HANDLE m_hTimer;

	CX264Encoder m_encoder;
	CDxgiCapture m_dxgiCapture;

	//volatile DWORD	m_dwMaxFps;
	//DWORD		    m_Quality;
	
	DWORD		    m_flags;
	volatile long   m_TimerMutex;

	//clipboard listener.
	HANDLE		    m_ClipbdListenerThread;
	HWND		    m_hClipbdListenWnd;

	static void CALLBACK ClipdListenProc(CRemoteDesktop*pThis);
	static LRESULT CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static void CALLBACK TimerCallback(PVOID lpParam, BOOLEAN TimerOrWaitFired);

public:

	static volatile unsigned int nInstance;

	

	void OnClose();
	void OnOpen();

	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);


	void StopCapture();
	void OnStartCapture(int monitor,UINT fps,UINT quality);
	
	//void OnScreenShot();
	void OnNextFrame();
	void OnControl(CtrlParam*Param);
	void OnSetFlag(DWORD dwFlag);

	int  getCurCursorIdx(HCURSOR hCursor);

	void OnSetClipbdText(TCHAR*szText);
	void SetClipbdText(TCHAR*szText);

	CRemoteDesktop(CClient *pClient, Module * owner);
	~CRemoteDesktop();
};

