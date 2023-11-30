#include "stdafx.h"

#include "SocksProxySrv.h"
#include "SocksProxyUDPListener.h"


CSocksProxyUDPListener::CSocksProxyUDPListener(CSocksProxySrv*pHandler,UINT32 ClientID)
{
	m_pHandler = pHandler;
	m_pHandler->Get();
	m_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	m_clientID = ClientID;
	m_clientAddr = NULL;
	m_addrLen = 0;

	memset(m_traffic, 0, sizeof(m_traffic));

	m_pHandler->debug_inc();
}

CSocksProxyUDPListener::~CSocksProxyUDPListener()
{
	m_pHandler->Put();

	if (m_clientAddr)
	{
		delete[]m_clientAddr;
		m_clientAddr = NULL;
	}
		
	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
}


BOOL CSocksProxyUDPListener::Run()
{
	BOOL bRet = TRUE;
	if (!RecvFrom(m_buff,sizeof(m_buff),NULL,NULL,0))
	{
		Close();
		bRet = FALSE;
	}
	return bRet;
}

void CSocksProxyUDPListener::OnSendTo(
	BYTE * lpData,
	UINT32 nTransferredBytes,
	const SOCKADDR *lpToAddr,
	int iToLen,
	void * lpParam,
	DWORD Error)
{
	SetEvent(m_hEvent);
}

/*
	UDP���ݸ�ʽ:

	type		length			value
	RSV			2				0x0000
	FRAG	    1               ��ǰ�ķֶκ�...

	ATYP		1				01: IPV4
	04: IPV6
	DST.PORT	2				dest.port

	DATA		xxx
*/
void CSocksProxyUDPListener::OnRecvFrom(
	BYTE * lpData,
	UINT32 nTransferredBytes,
	const SOCKADDR *lpFromAddr,
	int iFromLen,
	void * lpParam,
	DWORD Error)
{
	vec bufs[3];
	BYTE cmd = 0x3;

	if (Error || !nTransferredBytes)
	{
		Close();
		//dbg_log("udp recvfrom failed with error : %d ", Error);
		return;
	}

	if (nTransferredBytes <= 10)
	{
		//dbg_log("Invalid udp datagram\n");
		return;
	}

	//check rsv is valid ?
	if (lpData[0] || lpData[1])
	{
		//dbg_log("invalid rsv.\n");
		Close();
		return;
	}

	//���Էֶκ�
	if (lpData[2])
	{
		//dbg_log("not support FRAG,drop data packet\n");
		Close();
		return;
	}

	if (!m_clientAddr)
	{
		m_clientAddr = new BYTE[iFromLen];
		m_addrLen = iFromLen;
		memcpy(m_clientAddr, lpFromAddr, iFromLen);
	}
	else if (memcmp(m_clientAddr,lpFromAddr,iFromLen))
	{
		dbg_log("not matched source port.drop this packet\n");
		goto __continue;
	}

	//һ�� UDP Associate ������������м���udp�ͻ���????.
	//������ֻ����һ��?.....�������Ϊ��һ��.
	//�����ڵ�һ���յ�socket ������ʱ��,���� client�Ķ˿ں͵�ַ.(���浽 m_ClientAddr)
	//ֻ��һ���ͻ���,��������ͻ��˿��ܻ���ͬ�ĵ�ַ��������.

	lpData += 3;
	nTransferredBytes -= 3;

	bufs[0].lpData = &m_clientID;							//
	bufs[0].Size = sizeof(m_clientID);						//

	bufs[1].lpData = &cmd;
	bufs[1].Size = 1;

	bufs[2].lpData = lpData;
	bufs[2].Size = nTransferredBytes;

	m_pHandler->Send(SOCK_PROXY_DATA, bufs, 3);

	//update traffic.
	switch (*lpData)		//addr type.
	{
	case 0x1:
		m_traffic[1] += nTransferredBytes - 1 - 4 - 2;
		break;
	case 0x3:
		m_traffic[1] += nTransferredBytes - 1 - 1 - lpData[1] - 2;
		break;
	case 0x4:
		m_traffic[1] += nTransferredBytes - 1 - 16 - 2;
		break;
	}

__continue:
	RecvFrom(m_buff,sizeof(m_buff),NULL,NULL,0);
}


//forward data from remote to local.

/*
	type		length			value
	RSV			2				0x0000
	FRAG	    1               ��ǰ�ķֶκ�...

	//
	ATYP		1				01: IPV4										//���ݰ���Դ��ַ
	04: IPV6																	//���ݰ���Դ�˿ں�.
	DST.PORT	2				dest.port

	DATA		xxx
*/

void CSocksProxyUDPListener::OnProxyData(BYTE*lpData, UINT Size)
{
	m_SendBuf[0] = 0;
	m_SendBuf[1] = 0;
	m_SendBuf[2] = 0;

	if (m_clientAddr)
	{
		switch (((SOCKADDR*)m_clientAddr)->sa_family)
		{
		case AF_INET:
			m_SendBuf[3] = 0x1;
			memcpy(&m_SendBuf[4], &((SOCKADDR_IN*)m_clientAddr)->sin_addr, 4);
			memcpy(&m_SendBuf[8], &((SOCKADDR_IN*)m_clientAddr)->sin_port, 2);
			
			if (Size > (UDP_MAX_BUFF - 10))
			{
				return;
			}

			WaitForSingleObject(m_hEvent, INFINITE);
			memcpy(&m_SendBuf[10], lpData, Size);
			SendTo(m_SendBuf, Size + 10, NULL, (SOCKADDR*)m_clientAddr, m_addrLen, NULL, m_hEvent);
			break;
		default:
			break;
		}
		
		//update download traffic.
		m_traffic[0] += Size;
	}
}