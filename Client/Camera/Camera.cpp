#include "Camera.h"
#include "..\Common\json\json.h"



#ifdef _DEBUG
#pragma comment(lib,"jsond.lib")
#else
#pragma comment(lib,"json.lib")
#endif

#define STRSAFE_NO_DEPRECATE

volatile unsigned int CCamera::nInstance = 0;

CCamera::CCamera(CClient *pClient, Module * owner) :
CEventHandler(pClient, CAMERA){
	m_bStop = FALSE;
	m_hWorkThread = NULL;

	m_owner = owner;
	if (m_owner)
		get_module(m_owner);
}


CCamera::~CCamera()
{
	if (m_owner)
		put_module(m_owner);
}

void CCamera::OnOpen()
{
	if (InterlockedExchangeAdd(&nInstance, 1) > 0)
	{
		//已经有一个实例了.
		TCHAR szError[] = TEXT("One instance is already running");
		Send(CAMERA_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		Close();
		return;
	}

	const VideoInfoList&video_info = m_Grab.GetDeviceList();
	Json::FastWriter writer;
	Json::Value root;

	for (auto & device : video_info)
	{
		Json::Value size;
		for (auto&video_size : device.second)
		{
			size["width"] = video_size.first;
			size["height"] = video_size.second;
			root[device.first].append(size);
		}
	}
	string response = writer.write(root);
	Send(CAMERA_DEVICELIST, (void*)response.c_str(), response.length() + 1);
}

void CCamera::OnClose()
{
	//一定要停止捕捉,否则线程继续发送会崩掉.
	OnStop();
	InterlockedDecrement(&nInstance);
}

void CCamera::OnStart(const string &device_name,int width, int height)
{
	DWORD dwResponse[3] = { 0 };
	string data;
	Json::Value res;

	char szError[0x100];
	
	//停止发送。
	m_Grab.StopGrab();						//停止捕捉
	//清理未释放的线程资源.
	InterlockedExchange(&m_bStop, TRUE);	//停止发送
	//关闭上一次未释放的资源.
	if (m_hWorkThread){
		WaitForSingleObject(m_hWorkThread, INFINITE);		//等待线程退出
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	//取消停止标记.
	InterlockedExchange(&m_bStop, FALSE);


	int code = m_Grab.GrabberInit(device_name,width, height, szError);

	res["code"] = code;
	res["err"] = szError;
	res["width"] = width;
	res["height"] = height;

	data = Json::FastWriter().write(res);
	Send(CAMERA_VIDEOSIZE, (void*)data.c_str(), data.length() + 1);
	
	if (code)
	{
		return;			//Grab init failed....
	}

	if (TRUE == m_Grab.StartGrab())
	{
		m_hWorkThread = CreateThread(
			0,
			0, 
			(LPTHREAD_START_ROUTINE)WorkThread, 
			(LPVOID)this, 
			0, 
			0);
	}
}

void CCamera::WorkThread(CCamera*pThis)
{
	BOOL bStop = FALSE;

	while (!InterlockedExchange(&pThis->m_bStop, FALSE))
	{
		BYTE *lpFrame = NULL;
		UINT32 uFrameSize = NULL;

		bStop = !pThis->m_Grab.GetFrame(&lpFrame, &uFrameSize);

		if (!bStop)
		{
			pThis->Send(CAMERA_FRAME, lpFrame, uFrameSize);	//编码后的,不要压缩.
		}
		//采集速度好像就是30fps/s,这里不用限制了,
		//网络延迟加队列缓存造成服务器不能实时显示画面,图像越大,由于带宽较小,延迟越高
		/*DWORD dwUsedTime = (GetTickCount() - dwLastTime);
		while ((GetTickCount() - dwLastTime) < (1000 / 30))
			Sleep(1);*/
	}
	InterlockedExchange(&pThis->m_bStop, TRUE);
}

void CCamera::OnStop()
{
	m_Grab.StopGrab();						//停止捕捉
	InterlockedExchange(&m_bStop, TRUE);	//停止发送

	if (m_hWorkThread)
	{
		WaitForSingleObject(m_hWorkThread,INFINITE);		//等待线程退出
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	
	//关闭完成.
	m_Grab.GrabberTerm();
	Send(CAMERA_STOP_OK, 0, 0);
}

void CCamera::OnEvent(UINT32 e, BYTE*lpData,UINT Size)
{
	switch (e)
	{
	case CAMERA_START:
		do{
			Json::Value root;
			Json::Reader reader;
			if (reader.parse((char*)lpData, root))
			{
				OnStart(
					root["device"].asString(),
					root["width"].asInt(),
					root["height"].asInt());
			}
		} while (0);
		break;
	case CAMERA_STOP:
		OnStop();
		break;
	default:
		break;
	}
}