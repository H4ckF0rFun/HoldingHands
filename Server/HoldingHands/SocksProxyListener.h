#pragma once
#include "TCPSocket.h"

class CSocksProxySrv;
class CSocksProxyListener :
	public CTCPSocket
{
private:
	CSocksProxySrv * m_pHandler;
public:

	void OnAccept(
		SOCKET hClientSocket, 
		const SOCKADDR* RemoteAddr, 
		int AddrLen, 
		void * lpParam, 
		DWORD Error);

	CSocksProxyListener(CSocksProxySrv * pHandler);
	virtual ~CSocksProxyListener();
};

