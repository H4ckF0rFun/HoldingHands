#include "stdafx.h"
//#include "IOCPServer.h"
//#include "Packet.h"
//
//CIOCPServer *pS;
//DWORD interval;
//char Text[64];
//
//DWORD AcceptCount = 0;
//
//LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//	char Text[256];
//	switch (uMsg)
//	{
//	case WM_CREATE:
//		CIOCPServer::SocketInit();
//		pS = CIOCPServer::CreateServer();
//		pS->AsyncStartSrv(hWnd,10086);
//		SetTimer(hWnd, 10086, 1000, NULL);
//		break;
//	case WM_IOCP_SRV_START:
//		MessageBox(hWnd, L"服务器已开启!", L"Error", IDOK);
//		SetTimer(hWnd,10086,1000,NULL);
//		break;
//	case WM_IOCP_SRV_CLOSE:
//		MessageBox(hWnd, L"服务器已关闭!", L"Error", IDOK);
//		KillTimer(hWnd, 10086);
//		break;
//	case WM_TIMER:
//		wsprintf(Text, L"Upload:%d KB/s Download:%d KB/s", pS->GetWriteSpeed(), pS->GetReadSpeed());
//		SetWindowTextW(hWnd,Text);
//		break;
//	case WM_CLOSE:
//		//DestroyWindow(hWnd);
//		pS->AsyncStopSrv(hWnd);
//		//CIOCPServer::SocketTerm();
//		printf("The server has been stoped!\n");
//		//PostQuitMessage(0);
//		break;
//	
//	default:
//		return DefWindowProc(hWnd, uMsg, wParam, lParam);
//	}
//	return 0;
//}
//
//int main()
//{
//
//	HINSTANCE hInstance = GetModuleHandle(NULL);
//	WNDCLASS wndclass = { 0 };
//	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
//	wndclass.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
//	wndclass.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
//	wndclass.hInstance = hInstance;
//	wndclass.lpszClassName = L"IOCPServerTest";
//	wndclass.lpfnWndProc = WndProc;
//
//	RegisterClass(&wndclass);
//	HWND hWnd = CreateWindow(L"IOCPServerTest",
//		L"IOCP", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
//		CW_USEDEFAULT, CW_USEDEFAULT, 500,600, 0, 0, hInstance, 0);
//
//	ShowWindow(hWnd, SW_SHOW);
//	UpdateWindow(hWnd);
//	MSG msg;
//
//	while (GetMessage(&msg, 0, 0, 0)){
//		TranslateMessage(&msg);
//		DispatchMessage(&msg);
//	}
//	//
//	printf("The server is closed.\n");
//	return 0;
//}