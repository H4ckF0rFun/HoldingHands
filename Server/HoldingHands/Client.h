#pragma once
#include "TCPSocket.h"
#include <stdint.h>

#define STATE_PKT_HEAD 0
#define STATE_PKT_BODY 1


//you should setNotifyWindow wen you get WM_CLIENT_LOGIN,
#define WM_CLIENT_LOGIN (WM_USER + 102)

class CEventHandler;

struct pkt_head
{
	uint32_t magic;
	uint32_t size;
};

struct vec
{
	const void *lpData;
	UINT32 Size;
};

class CClient :
	public CTCPSocket
{

private:
	friend class CEventHandler;

	HWND     m_hNotifyWindow;
	CEventHandler * m_pHandler;
	
	///////
	UINT     m_State;
	
	//pkt_head m_read_pkt_head;

	////received data length until now.
	//BYTE*    m_lpData;
	//UINT     m_BufSize;
	//UINT     m_DataLength;


	//recv Buffer;
	pkt_head * m_pkt;
	
	BYTE *     m_r_ptr;
	BYTE *     m_w_ptr;
	BYTE *     m_end_ptr;

	BYTE *     m_lpReadBuf;
	UINT32	   m_ReadBufSize;

	//
	BYTE *   m_lpWriteBuf;
	UINT     m_WriteBufSize;
	HANDLE   m_hEvent;


public:
	
	virtual void OnRecv(BYTE * lpData, UINT32 nTransferredBytes, void * lpParam, DWORD Error);
	virtual void OnSend(BYTE * lpData, UINT32 nTransferredBytes, void * lpParam, DWORD Error);

	void SetCallbackHandler(CEventHandler* h)
	{
		m_pHandler = h;
	}

	void Run();
	void Send(BYTE *lpData, UINT32 Size,BOOL Block = TRUE);
	void Send(vec * Bufs, int nBuf, BOOL Block = TRUE);

	void OnRecvCompletePacket(BYTE * lpData, UINT32 Size);

	LRESULT Notify(DWORD Msg, WPARAM wParam = 0, LPARAM lParam = 0, BOOL Sync = TRUE);
	void SetNotifyWindow(HWND hWnd){ m_hNotifyWindow = hWnd; };


	CClient(SOCKET hSock);
	virtual ~CClient();

};

