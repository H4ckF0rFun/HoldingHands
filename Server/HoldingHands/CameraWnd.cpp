#include "stdafx.h"
#include "CameraWnd.h"
#include "MainFrm.h"
#include <string>
#include "CameraSrv.h"
#include "json\json.h"
#include "utils.h"
#include "MainFrm.h"

#ifdef DEBUG
#pragma comment(lib,"jsond.lib")
#else
#pragma comment(lib,"json.lib")
#endif

#define MIN_WIDTH			460	
#define MIN_HEIGHT			240

CCameraWnd::CCameraWnd(CCameraSrv*pHandler):
	m_pHandler(pHandler),
	m_hdc(NULL),
	m_dwHeight(240),
	m_dwWidth(320),
	m_VideoWidth(0),
	m_VideoHeight(0),
	m_dwFps(0),
	m_dwLastTime(0),
	m_SizeCount(0),
	m_DeviceCount(0),
	m_CurrentSizeID(0)
{
	m_pHandler->Get();
}


CCameraWnd::~CCameraWnd()
{
	m_pHandler->Put();
}

BEGIN_MESSAGE_MAP(CCameraWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_MESSAGE(WM_CAMERA_DEVICELIST,OnDeviceList)
	ON_MESSAGE(WM_CAMERA_VIDEOSIZE, OnVideoSize)
	ON_MESSAGE(WM_CAMERA_FRAME,OnFrame)
	ON_MESSAGE(WM_CAMERA_ERROR,OnError)
	ON_WM_CLOSE()
	ON_WM_SYSCOMMAND()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


int CCameraWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	char szIP[0x100];
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDC*pDc = GetDC();
	pDc->SelectObject(GetStockObject(GRAY_BRUSH));

	m_hdc = pDc->m_hDC;

	//Set Window Text..
	m_pHandler->GetPeerAddress(szIP);
	m_Title.Format(TEXT("[%s] Camera"), CString(szIP));
	SetWindowText(m_Title);

	//set notify window...
	m_pHandler->SetNotifyWindow(GetSafeHwnd());
	return 0;
}


#define IDM_SCREENSHOT		0x0020
#define IDM_RECORD			0x0021
#define IDM_CAMERA_BEGIN	0x0030


LRESULT CCameraWnd::OnDeviceList(WPARAM wParam, LPARAM lParam)
{
	string json_res = (char*)wParam;
	Json::Reader reader;
	Json::Value root;

	if (!reader.parse(json_res, root))
	{
		MessageBox(TEXT("Parse Json Failed!"));
		return 0;
	}

	CMenu *pMenu = GetSystemMenu(FALSE);
	//解析数据..
	if (pMenu)
	{
		pMenu->AppendMenu(MF_STRING, IDM_SCREENSHOT, TEXT("Screenshot"));
		pMenu->AppendMenu(MF_STRING, IDM_RECORD, TEXT("Record"));
		pMenu->AppendMenu(MF_SEPARATOR);

		Json::Value::Members members = root.getMemberNames();
		for (auto it = members.begin(); it != members.end(); it++)
		{
			string device_name = *it;
			m_deviceName.Add(CString(device_name.c_str()));
			//Insert device name.
			CMenu VideoSizeMenu;
			VideoSizeMenu.CreateMenu();

			int iSize = root[device_name].size();
			
			for (int i = 0; i < iSize;i++)
			{
				Json::Value Size = root[device_name][i];
				CString strSize;
				int width = Size["width"].asInt();
				int height = Size["height"].asInt();
				strSize.Format(TEXT("%d x %d"), width, height);

				m_sizeToDevice.Add(m_DeviceCount);
				m_VideoSize.Add({ width, height });
				VideoSizeMenu.AppendMenu(MF_STRING, IDM_CAMERA_BEGIN + m_SizeCount, strSize);
				m_SizeCount++;
			}
			//插到最后的位置
			pMenu->InsertMenu(
				-1,
				MF_STRING | MF_POPUP | MF_BYPOSITION,
				(UINT)VideoSizeMenu.Detach(), 
				CString(device_name.c_str()));

			m_DeviceCount++;
		}
	}
	
	if (!m_SizeCount){
		MessageBox(TEXT("No Camera."), TEXT("Error"), MB_OK | MB_ICONINFORMATION);
		m_pHandler->Close();
		return 0;
	}
	//选择一个开启...
	PostMessage(WM_SYSCOMMAND, IDM_CAMERA_BEGIN);
	return 0;
}

void CCameraWnd::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (m_hdc)
	{
		::ReleaseDC(m_hWnd, m_hdc);
		m_hdc = NULL;
	}
	
	m_pHandler->SetNotifyWindow(NULL);
	m_pHandler->Close();


	DestroyWindow();
}

void CCameraWnd::PostNcDestroy()
{
	delete this;
}


void CCameraWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	switch (nID)
	{
	case IDM_SCREENSHOT:
		OnScreenShot();
		break;
	case IDM_RECORD:
		//暂未实现.
		break;
	default:
		if (nID >= IDM_CAMERA_BEGIN && nID < (IDM_CAMERA_BEGIN + m_SizeCount)){
			
			if (nID != m_CurrentSizeID)
			{
				int device_id = m_sizeToDevice[nID - IDM_CAMERA_BEGIN];
				CStringA deviceName(m_deviceName[device_id]);
				CString strSize;

				int width = m_VideoSize[nID - IDM_CAMERA_BEGIN].Width;
				int height = m_VideoSize[nID - IDM_CAMERA_BEGIN].Height;
				strSize.Format(TEXT("%d x %d"), width, height);

				m_pHandler->Start(deviceName, width, height);

				CMenu*pSysMenu = GetSystemMenu(FALSE);

				int iDeviceEnd = pSysMenu->GetMenuItemCount();
				int iDeviceBegin = iDeviceEnd - m_DeviceCount;
				//清除所有状态

				for (int i = iDeviceBegin; i < iDeviceEnd; i++)
				{
					CString MenuString;
					CStringA aMenuString;
					pSysMenu->GetMenuString(i, MenuString, MF_BYPOSITION);
					aMenuString = MenuString;
					pSysMenu->CheckMenuItem(i, MF_UNCHECKED | MF_BYPOSITION);

					//select device name.
					if (aMenuString == deviceName)
					{
						pSysMenu->CheckMenuItem(i, MF_CHECKED | MF_BYPOSITION);
					}

					CMenu*pMenu = pSysMenu->GetSubMenu(i);
					if (!pMenu)
						continue;

					for (int j = 0; j < pMenu->GetMenuItemCount(); j++)
					{
						pMenu->CheckMenuItem(j, MF_UNCHECKED | MF_BYPOSITION);

						if (pMenu->GetMenuItemID(j) == nID)
						{
							pMenu->CheckMenuItem(j, MF_CHECKED | MF_BYPOSITION);
						}
					}
				}
				m_CurrentSizeID = nID;
			}
		}
		else
		{
			CFrameWnd::OnSysCommand(nID, lParam);
		}
		break;
	}
}

void CCameraWnd::OnScreenShot()
{
	CMainFrame * pMainWnd = (CMainFrame*)AfxGetMainWnd();
	CString      SavePath = CString(pMainWnd->Config().cfg()["cam"]["screenshot_save_path"].asCString());
	CString      FileName;
	CString      OutputFile;

	FileName.Format(TEXT("\\%s.bmp"), CTime::GetTickCount().Format("%Y-%m-%d_%H_%M_%S").GetBuffer());

	OutputFile.Format(TEXT("%s\\%s"), SavePath, FileName);

	if (OutputFile[1] != ':')
	{
		CString csCurrentDir;
		csCurrentDir.Preallocate(MAX_PATH);
		GetProcessDirectory(csCurrentDir.GetBuffer());
		OutputFile = csCurrentDir + "\\" + OutputFile;
	}

	MakesureDirExist(OutputFile.GetBuffer(), TRUE);

	m_pHandler->ScreenShot(OutputFile.GetBuffer());
}

LRESULT CCameraWnd::OnVideoSize(WPARAM Width, LPARAM Height){
	//save video info.

	m_VideoHeight = Height;
	m_VideoWidth = Width;

	m_dwHeight = max(Height,MIN_HEIGHT);
	m_dwWidth = max(Width, MIN_WIDTH);

	m_Org.x = (m_dwWidth - m_VideoWidth) / 2;
	m_Org.y = (m_dwHeight - m_VideoHeight) / 2;

	//adjust window position
	CRect WndRect, ClientRect;
	GetWindowRect(WndRect);
	GetClientRect(ClientRect);
	WndRect.right += m_dwWidth - ClientRect.Width();
	WndRect.bottom += m_dwHeight - ClientRect.Height();
	//
	MoveWindow(WndRect);
	return 0;
}

LRESULT CCameraWnd::OnFrame(WPARAM wParam, LPARAM lParam)
{
	HDC hMdc = (HDC)wParam;
	BITMAP*pBmp = (BITMAP*)lParam;
	//
	
	if (m_VideoHeight < m_dwHeight ||
		m_VideoWidth < m_dwWidth)
	{
		
		CRect rect;
		rect.left = rect.top = 0;
		rect.right = max(m_dwWidth, m_VideoWidth);
		rect.bottom = max(m_dwHeight, m_VideoHeight);

		FillRect(m_hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	}

	//显示图像
	BitBlt(m_hdc, m_Org.x, m_Org.y, m_VideoWidth, m_VideoHeight, hMdc, 0, 0, SRCCOPY);
	m_dwFps++;

	if ((GetTickCount() - m_dwLastTime) >= 1000)
	{
		//更新FPS
		CString csNewTitle;
		csNewTitle.Format(
			TEXT("%s - [Fps: %d] (%d x %d)"), 
			m_Title.GetBuffer(),
			m_dwFps,
			m_VideoWidth, 
			m_VideoHeight);

		SetWindowText(csNewTitle);
		m_dwLastTime = GetTickCount();
		m_dwFps = 0;
	}
	return 0;
}

LRESULT CCameraWnd::OnError(WPARAM wParam, LPARAM lParam)
{
	//失败
	TCHAR *szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}

void CCameraWnd::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	//adjust window position
	CRect WndRect, ClientRect;
	GetWindowRect(WndRect);
	GetClientRect(ClientRect);
	WndRect.right += m_dwWidth - ClientRect.Width();
	WndRect.bottom += m_dwHeight - ClientRect.Height();

	MoveWindow(WndRect);
}


void CCameraWnd::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CRect WndRect, ClientRect;
	GetWindowRect(WndRect);
	GetClientRect(ClientRect);

	lpMMI->ptMaxSize.x = WndRect.Width() + m_dwWidth - ClientRect.Width();
	lpMMI->ptMaxSize.y = WndRect.Height() + m_dwHeight - ClientRect.Height();
	
	CFrameWnd::OnGetMinMaxInfo(lpMMI);
}
