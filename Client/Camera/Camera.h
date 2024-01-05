#pragma once
#include "EventHandler.h"
#include "CameraGrab.h"
#include "camera_common.h"
#include "module.h"

class CCamera :
	public CEventHandler
{
private:
	Module *        m_owner;
	CCameraGrab     m_Grab;
	volatile long	m_bStop;

	HANDLE		    m_hWorkThread;

public:
	static volatile unsigned int nInstance;

	void OnStart(const string &device_name, int width, int height);
	void OnStop();

	void OnOpen();
	void OnClose();
	void OnEvent(UINT32 e, BYTE*lpData, UINT Size);

	CCamera(CClient *pClient,Module * owner);

	void static WorkThread(CCamera*pThis);

	~CCamera();
};

