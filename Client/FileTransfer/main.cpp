#include "IOCP.h"
#include "Client.h"
#include "FileTrans.h"


extern "C" __declspec(dllexport)
int  ModuleEntry(
	CIOCP * iocp,
	Module * owner,
	char* szServerAddr,
	unsigned short uPort,
	LPVOID ArgList[])

{
	int err = 0;
	CClient * client = new CClient;
	UINT32 Duty      = (UINT32)ArgList[0];
	TCHAR * SrcDir   = (TCHAR*)ArgList[1];
	TCHAR * DestDir  = (TCHAR*)ArgList[2];
	TCHAR * FileList = (TCHAR*)ArgList[3];

	CFileTrans * ft = new CFileTrans(client, owner,Duty, SrcDir, DestDir, FileList);

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
	ModuleEntry(iocp,NULL, "192.168.237.1", 10086, NULL);

	Sleep(INFINITE);
}
#endif