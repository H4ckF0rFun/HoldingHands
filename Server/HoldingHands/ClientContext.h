#pragma once
#include <afx.h>
#include <winsock2.h>
#include <MSWSock.h>
#include "Packet.h"
class CPacket;

#define REMOTE_AND_LOCAL_ADDR_LEN		(2 * (sizeof(sockaddr)+16))

class CMsgHandler;
class CClientContext
{
private:
	friend class CManager;
	friend class CIOCPServer;
	friend	class  CMsgHandler;
	//
	CIOCPServer*	m_pServer;

	CManager *		m_pManager;
	//
	char		m_szPeerAddr[64];
	char		m_szSockAddr[64];
	USHORT		m_PeerPort;
	USHORT		m_SockPort;
	//
	//Handler
	DWORD			m_Identity;								//功能
	//
	SOCKET			m_ClientSocket;	
	//与接受有关的buffer
	WSABUF			m_wsaReadBuf;							//每次投递请求时用于储存数据信息
	CPacket			m_ReadPacket;							//每次投递请求时用于储存数据信息	
	DWORD			m_dwRead;								//已经读取的数据
	//与发送有关的buffer
	WSABUF			m_wsaWriteBuf;
	CPacket			m_WritePacket;
	DWORD			m_dwWrite;								//已经发送的长度
	//
	POSITION		m_PosInList;							//保存在client list 中 context 的位置
	//sockaddr_in		m_sockaddr;
	CRITICAL_SECTION m_csCheck;								//防止重复关闭socket
	HANDLE			 m_SendPacketOver;						//是否发送完毕
	//
public:
	//发送一个包,失败返回-1,成功返回0
	int SendPacket(DWORD command,const char*data, int len,DWORD dwFlag);
	//断开连接
	void Disconnect();


	//ReadIO请求完成
	void OnReadComplete(DWORD nTransferredBytes, DWORD dwFailedReason);
	//Write IO请求完成
	void OnWriteComplete(DWORD nTransferredBytes, DWORD dwFailedReason);
	//断开连接
	void OnClose();
	void OnConnect();

	//
	void GetPeerName(char* Addr, USHORT&Port);
	void GetSockName(char* Addr, USHORT&Port);

	CClientContext(SOCKET ClientSocket,CIOCPServer *pServer,CManager*pManager);
	~CClientContext();
};

