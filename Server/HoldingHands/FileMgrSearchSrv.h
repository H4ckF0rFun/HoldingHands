#pragma once
#include "EventHandler.h"

#define FILEMGR_SEARCH			('S'|('R'<<8)|('C'<<16)|('H'<<24))

#define FILE_MGR_SEARCH_SEARCH		(0xaca1)

#define FILE_MGR_SEARCH_STOP		(0xaca2)

#define FILE_MGR_SEARCH_FOUND		(0xaca3)

#define FILE_MGR_SEARCH_OVER		(0xaca4)


//Notify Message
// CFileMgrSearchDlg �Ի���
#define WM_FILE_MGR_SEARCH_FOUND		(WM_USER + 195)
#define WM_FILE_MGR_SEARCH_OVER			(WM_USER + 196)
#define WM_FILE_MGR_SEARCH_ERROR		(WM_USER + 197)

class CFileMgrSearchDlg;

class CFileMgrSearchSrv :
	public CEventHandler
{
public:
	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnOpen();				//��socket���ӵ�ʱ������������
	//�����ݵ����ʱ���������������.
	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);;

	void OnFound(TCHAR*Buffer);
	void OnOver();

	void Search(TCHAR*szParams);
	void Stop();

	CFileMgrSearchSrv(CClient *pClient);
	~CFileMgrSearchSrv();
};

