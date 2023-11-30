
#include "Audio.h"
#include "Client.h"
#include "utils.h"

int worker_thread_init(void * lpParam)
{
	HRESULT hr = CoInitialize(0);
	if (FAILED(hr)){
		return -1;
	}

	return 0;
}

void worker_thread_fini(void * lpParam)
{
	CoUninitialize();
}


extern "C" __declspec(dllexport)
void  ModuleEntry(
CIOCP * iocp,
char* szServerAddr,
unsigned short uPort,
void *lpParam)

{
	CClient * client = new CClient;
	CAudio * audio = new CAudio(client);

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

	CIOCP * iocp = new CIOCP(0, worker_thread_init, worker_thread_fini, NULL);
	iocp->Create();

	//
	ModuleEntry(iocp, "127.0.0.1", 10086, NULL);

	Sleep(INFINITE);
}
#endif