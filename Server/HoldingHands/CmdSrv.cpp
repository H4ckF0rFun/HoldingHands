#include "stdafx.h"
#include "CmdSrv.h"

CCmdSrv::CCmdSrv(CClient*pClient) :
CEventHandler(pClient,CMD)
{
}


CCmdSrv::~CCmdSrv()
{
	dbg_log("CCmdSrv::~CCmdSrv()");
}

void CCmdSrv::OnClose()
{
	Notify(WM_CMD_ERROR, (WPARAM)TEXT("Connection close.."));
}

void CCmdSrv::OnOpen()
{
	int opt = 1;
	setsockopt(m_pClient->GetSocket(), SOL_SOCKET, TCP_NODELAY, (char*)&opt, sizeof(opt));
}


void CCmdSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	case CMD_BEGIN:
		OnCmdBegin(*((DWORD*)lpData));
		break;
	case CMD_RESULT:
		OnCmdResult((TCHAR*)lpData);
		break;
	default:
		break;
	}
}

void CCmdSrv::OnCmdResult(TCHAR*szBuffer)
{
	Notify(WM_CMD_RESULT, (WPARAM)szBuffer);
}

void CCmdSrv::OnCmdBegin(DWORD dwStatu)
{
	//∆Ù”√ ‰»Î.
	if (dwStatu == 0)
	{
		Notify(WM_CMD_BEGIN);
	}
	else
	{
		Notify(WM_CMD_ERROR, (WPARAM)TEXT("remote shell initialize failed."));
	}
}
