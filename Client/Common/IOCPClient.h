#pragma once
#include "Packet.h"
#include <WinSock2.h>
#include <MSWSock.h>
#include <stdio.h>
#include <string>
#include <ASSERT.H>
#define IO_CONNECT			0
#define IO_READ				1
#define IO_WRITE			2
#define IO_CLOSE			3
//--------------------------------------------------------


using std::string;

class CPacket;
class CManager;

struct OVERLAPPEDPLUS
{
	OVERLAPPED	m_overlapped;
	DWORD		m_IOtype;
	BOOL		m_bManualPost;			//用来记录是否为手动触发。

	OVERLAPPEDPLUS(DWORD IO_TYPE, BOOL bManualPost = FALSE)
	{
		m_IOtype = IO_TYPE;
		m_bManualPost = bManualPost;
		m_overlapped.hEvent = 0;
		m_overlapped.Internal = 0;
		m_overlapped.InternalHigh = 0;
		m_overlapped.Offset = 0;
		m_overlapped.OffsetHigh = 0;
		m_overlapped.Pointer = 0;
	}
};


#define HEART_BEAT		0x00abcdef
class CIOCPClient
{
	friend class CManager;
public:
	static LPFN_CONNECTEX lpfnConnectEx;

private:
	volatile long	m_bHeartBeatReply;
	//自动重新连接
	BOOL		m_isConnecting;		//是否正在连接
	BOOL		m_bReConnect;		//是否重新连接
	UINT		m_MaxTryCount;		//最大尝试次数
	UINT		m_FailedCount;		//连续失败次数.

	//IOCP
	HANDLE		m_hCompletionPort;	//emmmm还是用一个线程吧,两个太复杂了.
	//---------------------------------------------------------
	//socket
	SOCKET		m_ClientSocket;		//socket
	//IOCP 接收数据
	CPacket		m_ReadPacket;
	WSABUF		m_wsaReadBuf;
	DWORD		m_dwRead;
	//IOCP 发送数据.
	CPacket		m_WritePacket;
	WSABUF		m_wsaWriteBuf;
	DWORD		m_dwWrite;

	CRITICAL_SECTION	m_csCheck;
	HANDLE		m_SendPacketOver;
	//
	CManager	* m_pManager;

	///////
	string			m_sServerAddr;
	unsigned short m_ServerPort;

	//投递一个ReadIO请求
	void PostRead();
	void PostWrite();
	//处理消息。
	void OnConnect();
	void OnReadComplete(DWORD nTransferredBytes, BOOL bEOF);
	void OnWriteComplete(DWORD nTransferredBytes, BOOL bEOF);
	void OnClose();
	//重新连接
	BOOL TryConnect();
	//Send data
	
	HANDLE	m_hWorkThreads[2];
	void	IocpRun();
	static unsigned int __stdcall WorkThread(void*);
public:
	//只用调用一次就行
	static BOOL SocketInit();
	static void SocketTerm();
	//
	//发送一个包
	void SendPacket(DWORD command, const char*data, int len,DWORD dwFlag);
	void Disconnect();

	void Run();
	//
	CIOCPClient(CManager*pManager, const char*ServerAddr, int ServerPort = 10086, 
		BOOL bReConnect = FALSE, UINT MaxTryCount = INFINITE);
	~CIOCPClient();
};

