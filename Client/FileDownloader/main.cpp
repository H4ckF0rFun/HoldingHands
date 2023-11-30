#include "IOCP.h"
#include "Client.h"
#include "..\FileManager\FileDownloader.h"


extern "C" __declspec(dllexport)
void  ModuleEntry(
CIOCP * Iocp,
char* szServerAddr,
unsigned short uPort,
LPVOID ArgList[])

{
	CClient * client = new CClient;
	UINT32 flags = (UINT32)ArgList[0];
	TCHAR * url = (TCHAR*)ArgList[1];
	TCHAR * savePath = (TCHAR*)ArgList[2];

	CFileDownloader * fd = new CFileDownloader(client, flags, savePath, url);

	if (!client->Create())
	{
		dbg_log("client->Create() failed");
		goto __failed__;
	}

	if (!client->Bind(0))
	{
		dbg_log("client->Bind() failed");
		goto __failed__;
	}

	if (!Iocp->AssociateSock(client))
	{
		dbg_log("iocp->AssociateSock(client) failed");
		goto __failed__;
	}

	if (!client->Connect(szServerAddr, uPort, NULL, NULL))
	{
		dbg_log("client->Connect() failed");
		goto __failed__;
	}
	client->Put();
	return;

__failed__:
	client->Close();
	client->Put();
	return;
}

#ifdef _DEBUG
int main(){

	WSADATA wsadata;

	if (WSAStartup(MAKEWORD(2, 0), &wsadata))
	{
		dbg_log("WSAStartup failed with error : %d", WSAGetLastError());
		exit(1);
	}

	CIOCP * iocp = new CIOCP;
	iocp->Create();

	//
	ModuleEntry(iocp, "127.0.0.1", 10086, NULL);

	Sleep(INFINITE);
}
#endif