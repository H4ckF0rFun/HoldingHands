#include "EventHandler.h"
#include <client.h>

CEventHandler::CEventHandler(CClient * pClient, UINT32 ModuleID)
{
	m_ID = ModuleID;
	m_pClient = pClient;
	m_pClient->SetCallbackHandler(this);
}

CEventHandler::~CEventHandler()
{

}

UINT CEventHandler::GetPeerAddress(char* szIP)
{
	SOCKADDR_IN addr = { 0 };
	int namelen = sizeof(addr);
	UINT Port = 0;

	*szIP = 0;

	if (m_pClient->GetPeerName((SOCKADDR*)&addr, &namelen))
	{
		Port = ntohs(addr.sin_port);
		lstrcpyA(szIP, inet_ntoa(addr.sin_addr));
	}
	return Port;
}

void  CEventHandler::Close()
{
	m_pClient->Close();
}

void CEventHandler::Get()
{
	m_pClient->Get();
}

void CEventHandler::Put()
{
	m_pClient->Put();
}

void CEventHandler::Send(UINT32 e, const void *lpData, UINT32 Size)
{
	vec Bufs[2];
	Bufs[0].lpData = (BYTE*)&e;
	Bufs[0].Size = sizeof(e);

	Bufs[1].lpData = lpData;
	Bufs[1].Size = Size;

	m_pClient->Send(Bufs, 2);
}

void CEventHandler::Send(UINT32 e, vec * Bufs, int nBuf)
{
	vec TempBufs[256];
	vec * tempBufs = TempBufs;

	if (nBuf >= 256)
		tempBufs = new vec[nBuf + 1];

	tempBufs[0].lpData = (BYTE*)&e;
	tempBufs[0].Size = sizeof(e);

	RtlCopyMemory(&tempBufs[1], Bufs, sizeof(vec) * nBuf);
	
	//send data.
	m_pClient->Send(tempBufs, nBuf + 1);

	//release tempBufs if it is dynamically allocated.
	if (tempBufs != TempBufs)
		delete[] tempBufs;

	return;
}

LRESULT	CEventHandler::Notify(UINT Message, WPARAM wParam, LPARAM lParam , BOOL bWait)
{
	if (m_hNotifyWindow == NULL)
		return 0;

	if (bWait)
		return SendMessageA(m_hNotifyWindow, Message, wParam, lParam);
	else
		return PostMessageA(m_hNotifyWindow, Message, wParam, lParam);
}