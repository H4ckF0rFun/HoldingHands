#pragma once

#include "file_search_common.h"
#include "EventHandler.h"




//Notify Message
// CFileMgrSearchDlg �Ի���
#define WM_FILE_MGR_SEARCH_FOUND		(WM_USER + 195)
#define WM_FILE_MGR_SEARCH_OVER			(WM_USER + 196)
#define WM_FILE_MGR_SEARCH_ERROR		(WM_USER + 197)

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

