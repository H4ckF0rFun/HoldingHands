#include "stdafx.h"
#include "SocksProxyListener.h"
#include "SocksProxyTcp.h"
#include "SocksProxySrv.h"

CSocksProxyListener::CSocksProxyListener(CSocksProxySrv * pHandler)
{
	m_pHandler = pHandler;
	m_pHandler->Get();
}


CSocksProxyListener::~CSocksProxyListener()
{
	m_pHandler->Put();
}


void CSocksProxyListener::OnAccept(
	SOCKET hClientSocket, 
	const SOCKADDR* RemoteAddr, 
	int AddrLen, 
	void * lpParam, 
	DWORD Error)
{
	CSocksProxyTcp * client = NULL;
	BOOL bAssociated = FALSE;

	if (Error)
	{
		dbg_log("accept failed with error : %d", Error);
	}

	if (hClientSocket != INVALID_SOCKET)
	{
		client = new CSocksProxyTcp(hClientSocket, m_pHandler);
		
		_read_lock(&m_rw_spinlock);
		
		if (m_flag & FLAG_ASSOCIATED)
			bAssociated = m_Iocp->AssociateSock(client);
		
		_read_unlock(&m_rw_spinlock);

		if (bAssociated)
		{
			client->OnOpen();
		}
	}

	if (client)
		client->Put();

	if (!Accept(NULL))
	{
		dbg_log("accept next failed");
		Close();
	}
}