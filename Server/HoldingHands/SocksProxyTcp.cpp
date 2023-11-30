#include "stdafx.h"
#include "SocksProxySrv.h"
#include "SocksProxyTcp.h"
#include "SocksProxyUDPListener.h"


CSocksProxyTcp::CSocksProxyTcp(SOCKET hSocket, CSocksProxySrv *pHandler) :
CTCPSocket(hSocket, FLAG_ACCEPTED)
{
	m_pHandler = pHandler;
	m_pHandler->Get();

	m_ClientID = -1;
	
	m_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	m_r_ptr = m_w_ptr = m_buff;
	
	m_Ver = m_pHandler->GetSocksVersion();

	m_Cmd = -1;
	m_ConnectPort = -1;
	m_AddrType = -1;
	m_AddrLength = -1;

	m_udpListener = NULL;

	memset(m_traffic, 0, sizeof(m_traffic));

	switch (m_Ver)
	{
	case 0x4:
		m_State = SOCKS_PROXY_REQUEST;
		m_Substate = SOCKS4_VER;
		break;
	case 0x5:
		m_State = SOCKS_PROXY_HANDSHAKE;
		m_Substate = SOCKS5_VER;
		break;
	}

}


CSocksProxyTcp::~CSocksProxyTcp()
{
	if (m_ClientID != -1)
		m_pHandler->FreeClientID(m_ClientID);

	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	if (m_udpListener)
	{
		m_udpListener->Close();
		m_udpListener->Put();
		m_udpListener = NULL;
	}

	m_pHandler->Put();
}


/***************************************** sequential call **********************************************/
void CSocksProxyTcp::OnRecv(
	BYTE * lpData,
	UINT32 nTransferredBytes,
	void * lpParam,
	DWORD Error)
{
	if (Error || !nTransferredBytes)
	{
		Close();
		OnClose();
		return;
	}

	m_w_ptr += nTransferredBytes;

	//handle recv data.
	SockProxyHandle();

	//Continue to recv.
	if (m_w_ptr == &m_buff[TCP_MAX_BUFF])
		m_w_ptr = m_buff;

	//
	if (!Recv(m_w_ptr, &m_buff[TCP_MAX_BUFF] - m_w_ptr, NULL))
	{
		Close();
		OnClose();
	}
}

void CSocksProxyTcp::OnOpen()
{
	m_pHandler->Notify(WM_SOCKS_PROXY_CONNECTED, (WPARAM)this);

	if (!Recv(m_w_ptr, &m_buff[TCP_MAX_BUFF] - m_w_ptr, NULL))
	{
		Close();
		OnClose();
	}
}



void CSocksProxyTcp::OnClose()
{
	//dbg_log("CSocksProxyTcp [%p] OnClose", this);

	m_pHandler->Notify(WM_SOCKS_PROXY_CLOSED, (WPARAM)this);

	if (m_ClientID != -1)
		m_pHandler->Send(
		SOCK_PROXY_CLOSE, 
		&m_ClientID, 
		sizeof(m_ClientID));

}


void CSocksProxyTcp::HandshakeHandle()
{
	while (m_Substate < 3)
	{
		switch (m_Substate)
		{
		case SOCKS5_VER:
			if (m_r_ptr >= m_w_ptr)		//data is not enough;
				return;

			if (*m_r_ptr != 0x5)
			{
				Close();
				dbg_log("Client[%d] - Handshake Unmatched version : %d", m_ClientID, *m_r_ptr);
				return;
			}
			++m_r_ptr;
			++m_Substate;
			break;

		case SOCKS5_METHOD_LEN:
			if (m_r_ptr >= m_w_ptr)		//data is not enough;
				return;

			m_MethodLength = *m_r_ptr;
			++m_r_ptr;
			m_Substate++;
			break;
		
		case SOCKS5_AUTH_METHOD:
			if((m_r_ptr + m_MethodLength) > m_w_ptr){
				return;
			}
			memcpy(m_Method, m_r_ptr, m_MethodLength);
			m_r_ptr += m_MethodLength;
			m_Substate++;			///
			break;
		}
	}

	/*
			ver		1
			method  1
				:0x00  no auth
	*/
	BYTE res[2];
	res[0] = 0x5;
	res[1] = 0x0;

	Send(res, sizeof(res), NULL);

	m_State = SOCKS_PROXY_REQUEST;
	m_Substate = SOCKS5_VER;
	
	return;
}

void CSocksProxyTcp::RequestHandle()
{
	vec bufs[6];
	BYTE zero = 0;

	//Socks4 ,Socks4 only support tcp protocol.
	if (m_Ver == 0x4)
	{
		while (m_Substate < 5)
		{
			switch (m_Substate)
			{
			case SOCKS4_VER:
				if (m_r_ptr >= m_w_ptr)
					return;

				if (*m_r_ptr != 0x4)
				{
					dbg_log("Client[%d] - Unmatched version : %d", m_ClientID, *m_r_ptr);
					Close();
					return;
				}

				++m_r_ptr;
				++m_Substate;
				break;

			case SOCKS4_CMD:
				if (m_r_ptr >= m_w_ptr)
					return;

				if (*m_r_ptr != 0x1 && *m_r_ptr != 0x2)
				{
					dbg_log("Client[%d] - Unsupported Command : %d", m_ClientID, *m_r_ptr);
					Close();
					return;
				}

				m_Cmd = *m_r_ptr;
				++m_r_ptr;
				++m_Substate;
				break;

			case SOCKS4_PORT:
				if ((m_r_ptr + 2) > m_w_ptr)
					return;

				memcpy(&m_ConnectPort, m_r_ptr, 2);
				m_r_ptr += 2;
				m_Substate++;
				break;

			case SOCKS4_ADDR:
				if ((m_r_ptr + 4) > m_w_ptr)
					return;

				memcpy(m_ConnectAddr, m_r_ptr + 4, 4);
				m_ConnectAddr[4] = 0;

				m_r_ptr += 4;
				m_Substate++;
				break;

			case SOCKS4_USERID:
				if (m_r_ptr >= m_w_ptr)
					return;

				if (*m_r_ptr != NULL){
					BYTE userid_length = *m_r_ptr;
					if ((m_r_ptr + 1 + userid_length) > m_w_ptr)
						return;
					
					++m_r_ptr;

					while (userid_length){
						++m_r_ptr;
						--userid_length;
					}
				}
				else{
					++m_r_ptr;
				}

				m_Substate++;
				break;
			}
		}
		//
		m_AddrType = 0x1;		//IPV4
		m_AddrLength = 0x4;
	}
	else if (m_Ver == 0x5)
	{
		/*
			ver    1 : 0x5
			cmd	   1 : 
				0x1 Connect
				0x2 Bind
				0x3 UDP Associate.

			rsv    1 : 0x00
			atyp   1 
			dst.addr  varlength
			dst.port  2
		
		*/
		while (m_Substate < 4)
		{
			switch (m_Substate)
			{
			case SOCKS5_VER:
				if (m_r_ptr >= m_w_ptr)
					return;

				if (*m_r_ptr != 0x5){
					Close();
					dbg_log("Client[%d] - Unmatched version : %d", m_ClientID, *m_r_ptr);
					return;
				}
				++m_r_ptr;
				++m_Substate;
				break;

			case SOCKS5_CMD:
				if (m_r_ptr >= m_w_ptr)
					return;

				if (*m_r_ptr != 0x1 && *m_r_ptr != 0x2
					&& *m_r_ptr != 0x3)
				{
					dbg_log("Client[%d] - Unsupported Command : %d", m_ClientID, *m_r_ptr);
					Close();
					return;
				}
				m_Cmd = *m_r_ptr;

				++m_r_ptr;
				++m_Substate;
				break;
			case SOCKS5_RSV:
				if (m_r_ptr >= m_w_ptr)
					return;

				if (*m_r_ptr){
					Close();
					return;
				}

				++m_r_ptr;
				++m_Substate;
				break;
			case SOCKS5_ATYP:
				if (m_r_ptr >= m_w_ptr)
					return;


				if (*m_r_ptr == ADDRTYPE_IPV4){
					if ((m_r_ptr + 1 + 0x4 + 0x2) > m_w_ptr){
						return;
					}

					m_AddrType = *m_r_ptr;
					m_AddrLength = 0x4;

					++m_r_ptr;

					memcpy(&m_ConnectAddr,  m_r_ptr , 4);
					m_ConnectAddr[m_AddrLength] = 0;

					m_r_ptr += 4;

					memcpy(&m_ConnectPort, m_r_ptr, 2);
					m_r_ptr += 2;
					
					++m_Substate;
				}
				else if (*m_r_ptr == ADDRTYPE_IPV6){
					if ((m_r_ptr + 1 + 0x10 + 0x2) > m_w_ptr){
						return;
					}

					m_AddrType = *m_r_ptr;
					m_AddrLength = 0x10;

					++m_r_ptr;

					memcpy(&m_ConnectAddr, m_r_ptr, 0x10);
					m_ConnectAddr[m_AddrLength] = 0;

					m_r_ptr += 4;

					memcpy(&m_ConnectPort, m_r_ptr, 2);
					m_r_ptr += 2;

					++m_Substate;
				}
				else if (*m_r_ptr == ADDRTYPE_DOMAIN){		//域名
					if ((m_r_ptr + 1 + 1) > m_w_ptr)
					{
						return;
					}

					if ((m_r_ptr + 1 + 1 + m_r_ptr[1] + 2) > m_w_ptr)
					{
						return;
					}

					m_AddrType = *m_r_ptr;
					++m_r_ptr;

					m_AddrLength = *m_r_ptr;
					++m_r_ptr;

					//
					memcpy(m_ConnectAddr, m_r_ptr, m_AddrLength);
					m_ConnectAddr[m_AddrLength] = 0;

					m_r_ptr += m_AddrLength;

					memcpy(&m_ConnectPort, m_r_ptr, 2);
					m_r_ptr += 2;

					++m_Substate;
				}
				else{
					dbg_log("Client[%d] - Unsupported Address Type : %d", m_ClientID, *m_r_ptr);
					Close();
					return;
				}
			}
		}
	}
	/*
				field        size
				clientID	 4
				cmd			 1
				addrtype     1
				port         2
				addr         var
	*/
	//Alloc client ID
	if (m_ClientID == -1)
	{
		m_ClientID = m_pHandler->AllocClientID(this);
		if (m_ClientID == -1)
		{
			Close();
			return;
		}
	}

	bufs[0].lpData = &m_ClientID;
	bufs[0].Size = sizeof(m_ClientID);

	bufs[1].lpData = &m_Cmd;
	bufs[1].Size = sizeof(m_Cmd);

	bufs[2].lpData = &m_AddrType;
	bufs[2].Size = sizeof(m_AddrType);

	bufs[3].lpData = &m_ConnectPort;
	bufs[3].Size = sizeof(m_ConnectPort);

	bufs[4].lpData = &m_ConnectAddr;
	bufs[4].Size = m_AddrLength;

	bufs[5].lpData = &zero;
	bufs[5].Size = sizeof(zero);

	//dbg_log("Client[%d] Send Req,obj : %p", m_ClientID,this);
	m_pHandler->Send(SOCK_PROXY_REQUEST, bufs, 6);

	m_State++;
}

void CSocksProxyTcp::TcpForwardHandle()
{
	BYTE *lpData = m_r_ptr;
	UINT Size = m_w_ptr - m_r_ptr;
	vec  bufs[3] = { 0 };
	
	if (m_ClientID < 0)
	{
		//Invalid connection.
		Close();
		return;
	}

	if (!Size)
	{
		return;
	}

	if (m_udpListener)
	{
		dbg_log("recv tcp forward data,but cmd is udp forward.");

		m_udpListener->Close();
		m_udpListener->Put();
		m_udpListener = NULL;

		Close();
		return;
	}

	bufs[0].lpData = &m_ClientID;
	bufs[0].Size = sizeof(m_ClientID);
	
	bufs[1].lpData = &m_Cmd;
	bufs[1].Size = sizeof(m_Cmd);

	bufs[2].lpData = lpData;
	bufs[2].Size = Size;

	/*
		ClientID     cmd     data
	*/
	m_pHandler->Send(SOCK_PROXY_DATA, bufs, 3);
	m_traffic[1] += Size;
	
	//
	m_r_ptr = m_w_ptr = m_buff;
}

void CSocksProxyTcp::SockProxyHandle()
{
	//data we can use is buf[0:m_Pos];
	if (m_State == SOCKS_PROXY_HANDSHAKE)
	{
		HandshakeHandle();
	}

	if (m_State == SOCKS_PROXY_REQUEST)
	{
		RequestHandle();
	}
	
	if (m_State == SOCKS_PROXY_FORWARD)
	{
		TcpForwardHandle();
	}
}

/**********************************************************************/

/*
		ver              1
		rep              1
			0x00        success
			0x01        proxy server failed
			0x02        connection is not permitted
			0x03		网络不可达
			0x04        主机不可达
			0x05        连接拒绝
			0x06        TTL到期
			0x07        命令不支持
			0x08        地址类型不支持

		rsv              1 : 0x00
		atyp
		bind addr 
		bind port
*/

void CSocksProxyTcp::OnProxyResponse(BYTE*lpData, UINT32 Size)
{
	DWORD error = *(DWORD*)lpData;

	//socks5 , UDP.
	if (m_Ver == 0x5 && m_Cmd == 0x3 && !error)
	{
		m_udpListener = new CSocksProxyUDPListener(m_pHandler, m_ClientID);

		if (!m_udpListener->Create())
		{
			dbg_log("m_udpListener->Create() failed");
			m_udpListener->Close();
			m_udpListener->Put();
			m_udpListener = NULL;
		}
		else if (!m_udpListener->Bind(0, "0.0.0.0"))
		{
			dbg_log("m_udpListener->Bind() failed");
			m_udpListener->Close();
			m_udpListener->Put();
			m_udpListener = NULL;
		}
		else
		{
			BOOL bAssociated = FALSE;
			_read_lock(&m_rw_spinlock);

			if (m_flag & FLAG_ASSOCIATED)
			{
				if (m_Iocp->AssociateSock(m_udpListener))
					bAssociated = TRUE;
			}
			_read_unlock(&m_rw_spinlock);

			if (!bAssociated)
			{
				m_udpListener->Close();
				m_udpListener->Put();
				m_udpListener = NULL;
			}
			else{
				m_udpListener->Run();
			}
		}
	}

	//Response.
	if (m_Ver == 0x5)
	{
		//socks 5
		BYTE res[10] = { 0 };
		res[0] = 0x5;				//ver
		res[1] = 0x0;				//rep
		res[2] = 0x0;				//rsv
		res[3] = 0x1;				//atyp == ipv4

		if (error)
			res[1] = 0x1;		//代理服务器出错

		if (m_Cmd == 0x3 && !m_udpListener)
			res[1] = 0x1;		//代理服务器出错

		if (!error && m_Cmd == 0x3 && m_udpListener)
		{
			//udp ...
			SOCKADDR_IN ipv4_addr = { 0 };
			int namelen = sizeof(ipv4_addr);

			//获取UDPsocket 端口号.
			m_udpListener->GetSockName((SOCKADDR*)&ipv4_addr, &namelen);
			ipv4_addr.sin_addr.S_un.S_addr = m_pHandler->GetUDPAssociateAddr();
			memcpy(&res[4], &ipv4_addr.sin_addr, 4);		//
			memcpy(&res[8], &ipv4_addr.sin_port, 2);		//通过tcp 告诉客户端监听的IP和端口.

		}
		else if (!error && m_Cmd == 0x1)
		{
			//do nothing....
		}

		WaitForSingleObject(m_hEvent, INFINITE);
		memcpy(m_SendBuf, res, sizeof(res));
		Send(m_SendBuf, sizeof(res), NULL, NULL, m_hEvent);
		
	}
	else if (m_Ver == 0x4)				//socks 4
	{
		BYTE res[8] = { 0 };
		res[0] = 0x00;
		res[1] = 0x5a;			//Successed.

		if (error)
			res[1] = 0x5b;


		WaitForSingleObject(m_hEvent, INFINITE);
		memcpy(m_SendBuf, res, sizeof(res));
		Send(m_SendBuf, sizeof(res), NULL, NULL, m_hEvent);
	}

	if (error)
	{
		Close();
		return;
	}
}

void CSocksProxyTcp::OnProxyData(BYTE *lpData, UINT32 Size)
{

	//Forward remote data to local .
	if (m_udpListener)
	{
		m_udpListener->OnProxyData(lpData, Size);
		return;
	}
	
	//dbg_log("CSocksProxyTcp [%p] OnProxyData", this);
	WaitForSingleObject(m_hEvent, INFINITE);
	memcpy(m_SendBuf, lpData, Size);
	Send(m_SendBuf, Size, NULL, NULL, m_hEvent);
	
	//dbg_log("CSocksProxyTcp [%p] Send Success", this);
	//
	m_traffic[0] += Size;
}

void CSocksProxyTcp::GetTraffic(ULONGLONG traffic[2])
{
	if (m_udpListener)
	{
		m_udpListener->GetTraffic(traffic);
		return;
	}

	traffic[0] = m_traffic[0];
	traffic[1] = m_traffic[1];
}