#include "stdafx.h"
#include "Server.h"
#include "Client.h"
#include "dbg.h"

CServer::CServer()
{

}

CServer::~CServer()
{

}

void CServer::OnAccept(
	SOCKET hClientSocket, 
	const SOCKADDR* RemoteAddr,
	int AddrLen, 
	void * lpParam,
	DWORD Error)
{
	BOOL bAssociated;
	CClient * client = NULL;

	if (Error || hClientSocket == INVALID_SOCKET)
	{
		dbg_log("Accept client failed with : %d", Error);
	}

	//create a new client socket.
	//hold the lock before we use the m_Iocp pointer,because it may be modified by Iocp object.

	client = new CClient(hClientSocket);
	client->SetNotifyWindow(m_hNotifyWindow);

	_read_lock(&m_rw_spinlock);
	if (!(m_flag & FLAG_ASSOCIATED))		//listen socket has been closed..
	{
		_read_unlock(&m_rw_spinlock);
	}
	else
	{
		bAssociated = m_Iocp->AssociateSock(client);
		_read_unlock(&m_rw_spinlock);

		if (bAssociated)
			client->Run();	
	}

	//the client is hold by the iocp and threadpool now.
	//otherwise it will be released.
	client->Put();
	

	//accept next client.
	if (!Accept(NULL))
	{
		Close();
		dbg_log("accept next failed");
	}
}

LRESULT CServer::Notify(DWORD Msg, WPARAM wParam, LPARAM lParam, BOOL Sync)
{
	if (!m_hNotifyWindow)
		return 0;

	if (Sync)
		return SendMessage(m_hNotifyWindow, Msg, wParam, lParam);
	else
		return PostMessage(m_hNotifyWindow, Msg, wParam, lParam); 
}