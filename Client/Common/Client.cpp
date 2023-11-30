#include "stdafx.h"
#include "Client.h"
#include "dbg.h"
#include <stdint.h>
#include "EventHandler.h"

////////////////////////////////////////////////////////

#define MAGIC 0xdeadbeef

#define BUF_SIZE 0x1000

//max buffer is 2MB
#define MAX_BUF  0x200000


CClient::CClient()
{
	m_State  = 0;

	m_ReadBufSize = 0;
	m_lpReadBuf = NULL;
	m_r_ptr = NULL;
	m_w_ptr = NULL;
	m_end_ptr = NULL;
	m_pkt = NULL;

	//
	m_lpWriteBuf = 0;
	m_WriteBufSize = 0;
	m_hEvent = CreateEvent(0, 0,TRUE , 0);
	
	m_pHandler = NULL;

}


CClient::~CClient()
{
	if (m_pHandler)
	{
		delete m_pHandler;
		m_pHandler = NULL;
	}

	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	//read buff.
	if (m_lpReadBuf)
	{
		free(m_lpReadBuf);
		m_lpReadBuf = NULL;
	}

	if (m_lpWriteBuf)
	{
		free(m_lpWriteBuf);
		m_lpWriteBuf = NULL;
	}
}
//需要做 On Close.
void CClient::OnRecv(
	BYTE * lpData,
	UINT32 nTransferredBytes,
	void * lpParam,
	DWORD Error)
{
	UINT32 newSize;
	UINT64 pkt_off, r_off, w_off;

	//
	if (Error || !nTransferredBytes)
	{
		dbg_log("recv failed with error: %d, recv bytes : %d\n", Error, nTransferredBytes);
		Close();

		if (m_pHandler) m_pHandler->OnClose();
		return;
	}

	//
	m_w_ptr += nTransferredBytes;

	//m_buff 开头一定是header
	while (1)
	{
		if (m_State == STATE_PKT_HEAD)
		{
			if ((m_w_ptr - m_r_ptr) >= sizeof(pkt_head))
			{
				assert(m_pkt == NULL);

				m_pkt = (pkt_head*)m_r_ptr;
				m_r_ptr += sizeof(pkt_head);

				if (m_pkt->magic != MAGIC)
				{
					dbg_log("Invalid packet: %d", m_pkt->magic);
					Close();
					if (m_pHandler) m_pHandler->OnClose();
					return;
				}

				if (m_pkt->size > MAX_BUF)
				{
					dbg_log("request buffer is too long : %d", m_pkt->size);
					Close();
					if (m_pHandler) m_pHandler->OnClose();
					return;
				}

				//realloc buffer;
				if (sizeof(pkt_head) + m_pkt->size > m_ReadBufSize)
				{
					newSize = sizeof(pkt_head) + m_pkt->size;
					pkt_off = (BYTE*)m_pkt - m_lpReadBuf;
					r_off = m_r_ptr - m_lpReadBuf;
					w_off = m_w_ptr - m_lpReadBuf;

					dbg_log("realloc buffer %d -> %d", m_ReadBufSize, newSize);
					m_lpReadBuf = (BYTE*)realloc(m_lpReadBuf, newSize);
					m_ReadBufSize = newSize;

					m_r_ptr = m_lpReadBuf + r_off;
					m_w_ptr = m_lpReadBuf + w_off;
					m_pkt = (pkt_head*)(m_lpReadBuf + pkt_off);
					m_end_ptr = m_lpReadBuf + newSize;
				}
				m_State = STATE_PKT_BODY;
				continue;
			}

			if (m_r_ptr + sizeof(pkt_head) > m_end_ptr)
			{
				w_off = m_w_ptr - m_r_ptr;
				//
				memmove(m_lpReadBuf, m_r_ptr, m_w_ptr - m_r_ptr);
				m_r_ptr = m_lpReadBuf;
				m_w_ptr = m_lpReadBuf + w_off;
			}
			break;
		}
		if (m_State == STATE_PKT_BODY)
		{
			if ((m_w_ptr - m_r_ptr) >= m_pkt->size)
			{
				m_r_ptr += m_pkt->size;
				//recv a complete packet.
				OnRecvCompletePacket((BYTE*)(m_pkt + 1), m_pkt->size);
				m_pkt = NULL;
				m_State = STATE_PKT_HEAD;
				continue;
			}

			if (m_pkt->size + m_r_ptr > m_end_ptr)					//不能继续往后写了.
			{
				r_off = m_r_ptr - (BYTE*)m_pkt;
				w_off = m_w_ptr - (BYTE*)m_pkt;

				memmove(m_lpReadBuf, m_pkt, m_w_ptr - (BYTE*)m_pkt);
				m_pkt = (pkt_head*)m_lpReadBuf;
				m_r_ptr = m_lpReadBuf + r_off;
				m_w_ptr = m_lpReadBuf + w_off;
			}
			break;
		}
	}

	//Post Next Recv request,
	if (!Recv(m_w_ptr, m_end_ptr - m_w_ptr, NULL))
	{
		//if recv failed,we will not enter this function any more,
		//so we should call OnClose now.
		Close();
		if (m_pHandler) m_pHandler->OnClose();
	}
}

void CClient::OnSend(
	BYTE * lpData, 
	UINT32 nTransferredBytes, 
	void * lpParam, 
	DWORD Error)
{
	SetEvent(m_hEvent);
}

void CClient::OnRecvCompletePacket(BYTE * lpData, UINT32 Size)
{
	UINT32 e;

	//let m_pHandler process this packet.
	if (Size < 4)
	{
		dbg_log("Invalid data size : %d",Size);
		Close();
		return;
	}

	//call the event handler.
	e = *(uint32_t*)lpData;
	m_pHandler->OnEvent(e, lpData + 4, Size - 4);
}



void CClient::Run()
{
	m_ReadBufSize = 0x10000;
	m_lpReadBuf = (BYTE*)malloc(m_ReadBufSize);
	m_r_ptr = m_lpReadBuf;
	m_w_ptr = m_lpReadBuf;
	m_end_ptr = m_lpReadBuf + m_ReadBufSize;

	//begin recv.
	Recv(m_w_ptr, m_end_ptr - m_w_ptr, NULL);
}

void CClient::OnConnect(const SOCKADDR *Addr, int iAddrLen, void * lpParam, DWORD Error)
{
	UINT32 moduleID;
	if (Error)
	{
		dbg_log("connect failed with error : %d ", Error);
		Close();
		return;
	}

	//
	moduleID = m_pHandler->GetModuleID();
	Send((BYTE*)&moduleID,sizeof(moduleID));

	//
	m_pHandler->OnOpen();
	Run();
}

void CClient::Send(BYTE *lpData, UINT32 Size)
{
	pkt_head * pkt;
	//wait last send request release the write buffer.
	WaitForSingleObject(m_hEvent, INFINITE);
	Size += sizeof(pkt_head);

	if (m_WriteBufSize < Size)
	{
		m_lpWriteBuf = (BYTE*)realloc(m_lpWriteBuf, Size);
		m_WriteBufSize = Size;
	}
	
	pkt = (pkt_head*)m_lpWriteBuf;
	pkt->magic = MAGIC;
	pkt->size = Size - sizeof(pkt_head);

	memcpy(pkt + 1, lpData, Size - sizeof(pkt_head));

	CTCPSocket::Send(m_lpWriteBuf, Size, NULL);
}

void CClient::Send(vec * Bufs, int nBuf)
{
	UINT32 Size = 0;
	UINT32 copy = 0;
	pkt_head * pkt;

	WaitForSingleObject(m_hEvent, INFINITE);
	Size += sizeof(pkt_head);
	for (int i = 0; i < nBuf; i++)
		Size += Bufs[i].Size;

	if (m_WriteBufSize < Size)
	{
		m_lpWriteBuf = (BYTE*)realloc(m_lpWriteBuf, Size);
		m_WriteBufSize = Size;
	}

	pkt = (pkt_head*)m_lpWriteBuf;
	pkt->magic = MAGIC;
	pkt->size = Size - sizeof(pkt_head);
	copy += sizeof(pkt_head);

	for (int i = 0; i < nBuf; i++)
	{
		if (Bufs[i].Size)
		{
			memcpy(m_lpWriteBuf + copy, Bufs[i].lpData, Bufs[i].Size);
			copy += Bufs[i].Size;
		}
		//else skip this buffer;
	}

	CTCPSocket::Send(m_lpWriteBuf, Size, NULL);
}
