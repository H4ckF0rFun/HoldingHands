#include "IOCP.h"
#include "Client.h"
#include "FileManager.h"


extern "C" __declspec(dllexport)
void  ModuleEntry(
CIOCP * iocp,
char* szServerAddr,
unsigned short uPort,
void *lpParam)

{
	CClient * client = new CClient;
	CFileManager * fm = new CFileManager(client);

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

	if (!iocp->AssociateSock(client))
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
	ModuleEntry(iocp, "192.168.237.1", 10086, NULL);

	Sleep(INFINITE);
}
#endif