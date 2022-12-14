#include "stdafx.h"
#include "MiniFileTransSrv.h"
#include "MiniFileTransDlg.h"
#include "resource.h"

CMiniFileTransSrv::CMiniFileTransSrv(CManager*pManager) :
	CMsgHandler(pManager),
	m_dwCurFileIdentity(0),
	m_hCurFile(INVALID_HANDLE_VALUE),
	m_ullLeftFileLength(0),
	m_dwCurFileAttribute(0),
	m_pInit(nullptr),
	m_pDlg(nullptr),
	m_TransferFinished(FALSE)
{
	memset(m_Path, 0, sizeof(m_Path));

}


CMiniFileTransSrv::~CMiniFileTransSrv()
{
	if (m_pInit){
		free(m_pInit);
		m_pInit = nullptr;
	}
}



void CMiniFileTransSrv::OnClose()
{
	Clean();
	if (m_pDlg){
		if (m_pDlg->m_DestroyAfterDisconnect){
			//窗口先关闭的.
			m_pDlg->DestroyWindow();
			delete m_pDlg;
		}
		else{
			// pHandler先关闭的,那么就不管窗口了
			m_pDlg->m_pHandler = nullptr;
			if (!m_TransferFinished){
				m_pDlg->PostMessage(WM_MNFT_ERROR, (WPARAM)TEXT("Disconnect."));
			}
		}
		m_pDlg = nullptr;
	}
}

void CMiniFileTransSrv::OnOpen()
{
	m_pDlg = new CMiniFileTransDlg(this);
	ASSERT(m_pDlg->Create(IDD_FILETRANS, CWnd::GetDesktopWindow()));

	m_pDlg->ShowWindow(SW_SHOW);
}


void CMiniFileTransSrv::OnTransferFinished(){
	m_TransferFinished = TRUE;
	m_pDlg->SendMessage(WM_MNFT_TRANS_FINISHED);
}

//这里只有服务器才会调用.
void CMiniFileTransSrv::OnMINIInit(DWORD Read, char*Buffer)
{
	CMiniFileTransInit*pInit = (CMiniFileTransInit*)Buffer;

	if (m_pInit == NULL)
	{
		m_pInit = (CMiniFileTransInit*)malloc(Read);
		memcpy(m_pInit, Buffer, Read);
	}
	if (m_pInit->m_dwDuty == MNFT_DUTY_RECEIVER)
	{
		//服务端是接收端,开始请求文件.
		TCHAR*pPath = NULL;
		if (NULL != (pPath = wcsstr(m_pInit->m_szBuffer, L"\n")))
		{
			*pPath = NULL;
			//保存Dest路径.
			lstrcpy(m_Path, m_pInit->m_szBuffer);
			SendMsg(MNFT_TRANS_INFO_GET, (char*)(pPath+1),
				sizeof(TCHAR)*(lstrlen(pPath + 1) + 1));//发送\n之后的数据.
		}
		else
			Close();//无效的szBuffer;
	}
	//如果是发送端,等待对方来请求文件就可以了.
}

void CMiniFileTransSrv::OnWriteMsg(WORD Msg,DWORD dwSize, char*buffer)
{

}

void CMiniFileTransSrv::OnReadMsg(WORD Msg,  DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	/*******************************************************************************/
	//只有服务器才会受收到这个消息.
	case MNFT_INIT:
		OnMINIInit(dwSize, Buffer);
		break;
	/*******************************************************************************/
		//when we Get File from peer.
	case MNFT_TRANS_INFO_RPL:
		OnGetTransInfoRpl(dwSize, Buffer);							//获取到了传输信息
		break;
	case MNFT_FILE_INFO_RPL:
		OnGetFileInfoRpl(dwSize, Buffer);
		break;
	case MNFT_FILE_DATA_CHUNK_RPL:
		OnGetFileDataChunkRpl(dwSize, Buffer);
		break;
	case MNFT_TRANS_FINISHED:
		OnTransferFinished();
		break;
	/*******************************************************************************/
		//when we SendMsg File to peer.
	case MNFT_TRANS_INFO_GET:					//获取传输信息(文件个数,总大小)
		OnGetTransInfo(dwSize, Buffer);
		break;
	case MNFT_FILE_INFO_GET:					//对方开始请求文件了。
		OnGetFileInfo(dwSize, Buffer);
		break;
	case MNFT_FILE_DATA_CHUNK_GET:				//获取文件数据
		OnGetFileDataChunk(dwSize, Buffer);
		break;
	case MNFT_FILE_TRANS_FINISHED:				//当前文件传输完成(可能成功,也可能失败)
		OnFileTransFinished(dwSize, Buffer);
		break;
	}
}


void CMiniFileTransSrv::Clean()
{
	while (m_JobList.GetCount()){
		free(m_JobList.RemoveHead());
	}

	if (m_hCurFile != INVALID_HANDLE_VALUE){
		CloseHandle(m_hCurFile);
		m_hCurFile = INVALID_HANDLE_VALUE;
	}
}
//文件数据块到达了
void CMiniFileTransSrv::OnGetFileDataChunkRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Data_Chunk_Data*pRpl = (MNFT_File_Data_Chunk_Data*)Buffer;
	if (pRpl->dwFileIdentity != 
		m_dwCurFileIdentity){
		Close();
		return;
	}
	//写入文件数据
	DWORD dwWrite = 0;
	BOOL bFailed = (pRpl->dwChunkSize == 0 ||
		(!WriteFile(m_hCurFile, pRpl->FileDataChunk, pRpl->dwChunkSize, &dwWrite, NULL)) || 
		(dwWrite == 0));
	if (!bFailed)
	{
		//写入成功
		m_ullLeftFileLength -= dwWrite;
		if (m_ullLeftFileLength)
		{
			//继续接收
			MNFT_File_Data_Chunk_Data_Get fdcdg;
			fdcdg.dwFileIdentity = m_dwCurFileIdentity;
			SendMsg(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
		}
		//接收
		/*****************************************************************************/
		//交给窗口去处理
		m_pDlg->SendMessage(WM_MNFT_FILE_DC_TRANSFERRED, dwWrite, 0);
		/*****************************************************************************/
	}
	//如果失败了,或者接收完了.那么就接终止当前的传输,请求下一个文件.
	if (bFailed || !m_ullLeftFileLength)
	{
		//关闭句柄
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		//结束当前文件的传输.
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = bFailed ? MNFT_STATU_FAILED : MNFT_STATU_SUCCESS;
		SendMsg(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		/*****************************************************************************/
		//交给窗口去处理
		m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_FINISHED, ftf.dwStatu, 0);
		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//传输下一个
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		SendMsg(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
	}
}

void CMiniFileTransSrv::OnGetFileInfoRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Info *pRpl = (MNFT_File_Info*)Buffer;
	if (pRpl->dwFileIdentity != m_dwCurFileIdentity){
		Close();
		return;
	}
	//
	FileInfo*pFileInfo = &pRpl->fiFileInfo;

	m_dwCurFileAttribute = pFileInfo->Attribute;
	m_ullLeftFileLength = pFileInfo->dwFileLengthHi;
	m_ullLeftFileLength = (m_ullLeftFileLength << 32) + pFileInfo->dwFileLengthLo;
	//
	DWORD FullPathLen = lstrlen(m_Path) + 1 + lstrlen(pFileInfo->RelativeFilePath) + 1;
	PTCHAR FullPath = (TCHAR*)malloc(FullPathLen*sizeof(TCHAR));
	lstrcpy(FullPath, m_Path);
	lstrcat(FullPath, L"\\");
	lstrcat(FullPath, pFileInfo->RelativeFilePath);
	////交给窗口去处理
	m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_BEGIN, (WPARAM)pFileInfo, 0);
	if (!(pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY))
	{
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		if (m_ullLeftFileLength != 0)
		{
			//CreateFile.
			m_hCurFile = CreateFile(FullPath, GENERIC_WRITE, 0, NULL, 
				CREATE_ALWAYS, m_dwCurFileAttribute, NULL);
			//GetFileChunk.
			if (m_hCurFile != INVALID_HANDLE_VALUE)
			{
				//获取数据块
				MNFT_File_Data_Chunk_Data_Get fdcdg;
				fdcdg.dwFileIdentity = m_dwCurFileIdentity;
				SendMsg(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
			}
		}
	}
	else
	{
		CreateDirectoryW(FullPath, NULL);		//MakesureDirectoryExist
	}
	//文件长度为0,或者当前句柄打开失败,那么就请求下一个文件.
	if (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY || 
		m_ullLeftFileLength == 0 || m_hCurFile == INVALID_HANDLE_VALUE)
	{
		
		//结束当前文件的传输.
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY 
			|| m_ullLeftFileLength == 0) ? MNFT_STATU_SUCCESS : MNFT_STATU_FAILED;
		SendMsg(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		//
		/*****************************************************************************/
		m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_FINISHED, ftf.dwStatu, 0);
		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//传输下一个
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		SendMsg(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
	}
	free(FullPath);
}

void CMiniFileTransSrv::OnGetTransInfoRpl(DWORD Read, char*Buffer)
{
	MNFT_Trans_Info*pTransInfo = (MNFT_Trans_Info*)Buffer;
	//Save trans info.
	/************************************************************************/
	m_pDlg->SendMessage(WM_MNFT_TRANS_INFO, (WPARAM)pTransInfo, m_pInit->m_dwDuty);
	/************************************************************************/
	//Get First File,use timestamp as the file identity.
	MNFT_File_Info_Get fig;
	m_dwCurFileIdentity = GetTickCount();
	fig.dwFileIdentity = m_dwCurFileIdentity;
	SendMsg(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
}


//--------------------------------------下面这些是作为发送端要做的事情------------------------------------------
//结束当前文件的发送.
void CMiniFileTransSrv::OnFileTransFinished(DWORD Read, char*Buffer)
{
	MNFT_File_Trans_Finished*pftf = (MNFT_File_Trans_Finished*)Buffer;
	if (pftf->dwFileIdentity != m_dwCurFileIdentity){
		Close();
		return;
	}
	
	if (m_hCurFile != INVALID_HANDLE_VALUE){
		CloseHandle(m_hCurFile);
		m_hCurFile = INVALID_HANDLE_VALUE;
	}

	m_dwCurFileIdentity = 0;
	m_ullLeftFileLength = 0;
	m_dwCurFileAttribute = 0;
	//free memory;
	FileInfo*pFile = m_JobList.RemoveHead();
	if (pFile)
	{
		free(pFile);
	}
	/***********************************************************************************/
	m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_FINISHED, pftf->dwStatu, 0);
	/***********************************************************************************/
}
void CMiniFileTransSrv::OnGetFileDataChunk(DWORD Read, char*Buffer)
{
	if (((MNFT_File_Data_Chunk_Data_Get*)Buffer)->dwFileIdentity != m_dwCurFileIdentity)
	{
		//传输错误.断开
		Close();
		return;
	}
	//传输数据,64kb的缓冲区.
	MNFT_File_Data_Chunk_Data*pRpl = (MNFT_File_Data_Chunk_Data*)malloc(sizeof(DWORD) * 2 + 0x10000);
	pRpl->dwFileIdentity = m_dwCurFileIdentity;
	pRpl->dwChunkSize = 0;
	//如果读取结束了,也会返回TRUE,
	ReadFile(m_hCurFile, pRpl->FileDataChunk, 0x10000, &pRpl->dwChunkSize, NULL);
	//读了多少就发送多少。读取失败就是0,让对方  结束当前文件的读取.
	SendMsg(MNFT_FILE_DATA_CHUNK_RPL, (char*)pRpl, sizeof(DWORD) * 2 + pRpl->dwChunkSize);
	/**********************************************************************/
	m_pDlg->SendMessage(WM_MNFT_FILE_DC_TRANSFERRED, pRpl->dwChunkSize, 0);
	/**********************************************************************/
	free(pRpl);
}

//对方请求文件了
void CMiniFileTransSrv::OnGetFileInfo(DWORD Read, char*Buffer)
{
	MNFT_File_Info_Get *pfig = (MNFT_File_Info_Get*)Buffer;
	if (!m_JobList.GetCount()){
		SendMsg(MNFT_TRANS_FINISHED, 0, 0);//传输结束.
		OnTransferFinished();
		return;
	}

	//save file identity;
	m_dwCurFileIdentity = pfig->dwFileIdentity;
	//
	DWORD dwRplLen = sizeof(DWORD) * 4 +
		sizeof(TCHAR)* (lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1);

	MNFT_File_Info*pReply = (MNFT_File_Info*)malloc(dwRplLen);

	pReply->dwFileIdentity = m_dwCurFileIdentity;
	pReply->fiFileInfo.Attribute = m_JobList.GetHead()->Attribute;
	pReply->fiFileInfo.dwFileLengthLo = m_JobList.GetHead()->dwFileLengthLo;
	pReply->fiFileInfo.dwFileLengthHi = m_JobList.GetHead()->dwFileLengthHi;

	lstrcpy(pReply->fiFileInfo.RelativeFilePath, m_JobList.GetHead()->RelativeFilePath);
	/*************************************************************************/
	//交给窗口去处理
	m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_BEGIN, (WPARAM)&pReply->fiFileInfo, 0);
	/*************************************************************************/
	SendMsg(MNFT_FILE_INFO_RPL, (char*)pReply, dwRplLen);
	free(pReply);
	//正常情况下这里的代码是不会执行的.因为初始情况和每次结束发送后都会清除
	if (m_hCurFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCurFile);
		m_hCurFile = INVALID_HANDLE_VALUE;
	}
	//如果不是目录,那么打开对应的文件 
	if (!(m_JobList.GetHead()->Attribute & FILE_ATTRIBUTE_DIRECTORY))
	{
		DWORD dwFullPathLen = lstrlen(m_Path) + 1 + lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1;
		PTCHAR FullPath = (TCHAR*)malloc(dwFullPathLen*sizeof(TCHAR));
		lstrcpy(FullPath, m_Path);
		lstrcat(FullPath, L"\\");
		lstrcat(FullPath, m_JobList.GetHead()->RelativeFilePath);
		//打开文件句柄
		m_hCurFile = CreateFileW(FullPath, GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		//
		free(FullPath);
	}

}


//对方请求传输信息了,本地作为发送方
void CMiniFileTransSrv::OnGetTransInfo(DWORD dwRead, char*Buffer)
{
	TCHAR*pFileList = wcsstr((TCHAR*)Buffer, L"\n");
	if (pFileList == NULL || pFileList[1] == NULL)
	{
		//无效,直接断开
		Close();
		return;
	}
	pFileList[0] = 0;
	pFileList++;
	//保存路径.之后传输文件用到.因为JobList里面保存的都是相对路径.
	lstrcpy(m_Path, (TCHAR*)Buffer);
	//
	BFS_GetFileList((TCHAR*)Buffer, pFileList, &m_JobList);
	//	
	//回复TransInfo
	MNFT_Trans_Info TransInfoRpl;
	TransInfoRpl.dwFileCount = m_JobList.GetCount();
	TransInfoRpl.TotalSizeLo = m_JobList.GetTotalSize()&0xFFFFFFFF;
	TransInfoRpl.TotalSizeHi = (m_JobList.GetTotalSize() & 0xFFFFFFFF00000000) >> 32;
	//
	m_pDlg->SendMessage(WM_MNFT_TRANS_INFO, (WPARAM)&TransInfoRpl, m_pInit->m_dwDuty);
	SendMsg(MNFT_TRANS_INFO_RPL, (char*)&TransInfoRpl, sizeof(MNFT_Trans_Info));
}

//广度遍历获取文件列表.
void CMiniFileTransSrv::BFS_GetFileList(TCHAR*Path, TCHAR*FileNameList, FileInfoList*pFileInfoList)
{
	//empty file list:
	if (FileNameList == NULL || FileNameList[0] == 0 || Path == NULL || Path[0] == NULL)
		return;
	//
	//Get file Names:
	TCHAR*szFileNameEnd = FileNameList;
	TCHAR*szFileName = NULL;

	FileInfoList IterQueue;
	//跳过\n
	while (szFileNameEnd[0] && szFileNameEnd[0] == L'\n')
		szFileNameEnd++;

	while (szFileNameEnd[0])
	{
		szFileName = szFileNameEnd;
		//找到结尾
		while (szFileNameEnd[0] && szFileNameEnd[0] != L'\n')
			szFileNameEnd++;

		//save file 
		DWORD dwNameLen = ((DWORD)szFileNameEnd - (DWORD)szFileName)/sizeof(TCHAR);

		DWORD FileInfoLen = 3 * sizeof(DWORD) + (dwNameLen + 1) * sizeof(TCHAR);
		FileInfo*pNewFileInfo = (FileInfo*)malloc(FileInfoLen);

		pNewFileInfo->Attribute = 0;
		pNewFileInfo->dwFileLengthLo = 0;
		pNewFileInfo->dwFileLengthHi = 0;
		memcpy(pNewFileInfo->RelativeFilePath, szFileName, dwNameLen * sizeof(TCHAR));
		pNewFileInfo->RelativeFilePath[dwNameLen] = 0;
		//Get File Attribute:
		//
		DWORD len = lstrlen(Path) + 1 + lstrlen(pNewFileInfo->RelativeFilePath) + 1;

		TCHAR* FullPath = (TCHAR*)malloc(len*sizeof(TCHAR));
		lstrcpy(FullPath, Path);
		lstrcat(FullPath, L"\\");
		lstrcat(FullPath, pNewFileInfo->RelativeFilePath);
		WIN32_FIND_DATAW fd;
		HANDLE hFirst = FindFirstFileW(FullPath, &fd);
		if (hFirst != INVALID_HANDLE_VALUE)
		{
			pNewFileInfo->Attribute = fd.dwFileAttributes;
			pNewFileInfo->dwFileLengthHi = fd.nFileSizeHigh;
			pNewFileInfo->dwFileLengthLo = fd.nFileSizeLow;
			//
			FindClose(hFirst);
			IterQueue.AddTail(pNewFileInfo);
		}
		else
		{
			free(pNewFileInfo);
		}
		free(FullPath);
		//NextFileName,跳过\n
		while (szFileNameEnd[0] && szFileNameEnd[0] == '\n')
			szFileNameEnd++;
	}
	//BFS
	while (IterQueue.GetCount())
	{
		pFileInfoList->AddTail(IterQueue.RemoveHead());
		FileInfo*pFile = pFileInfoList->GetTail();
		if (!(pFile->Attribute &FILE_ATTRIBUTE_DIRECTORY))
			continue;
		//It is a dir.browse it;
		DWORD len = lstrlen(Path) + 1 + lstrlen(pFile->RelativeFilePath) + 2 + 1;
		TCHAR* StartDir = (TCHAR*)malloc(len*sizeof(TCHAR));
		lstrcpy(StartDir, Path);
		lstrcat(StartDir, L"\\");
		lstrcat(StartDir, (pFile->RelativeFilePath));
		lstrcat(StartDir, L"\\*");
		//path\\filepath
		WIN32_FIND_DATAW fd = { 0 };
		HANDLE hFirst = FindFirstFileW(StartDir, &fd);
		BOOL bNext = TRUE;
		//trave this dir:
		while (hFirst != INVALID_HANDLE_VALUE && bNext)
		{
			if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
				(lstrcmp(fd.cFileName, L".") && lstrcmp(fd.cFileName, L"..")))
			{
				
				//allocate path
				DWORD NewPathLen = lstrlen(pFile->RelativeFilePath) + 1 + lstrlen(fd.cFileName) + 1;

				DWORD FileInfoLen = 3*sizeof(DWORD) + NewPathLen * sizeof(TCHAR);
				
				FileInfo*pNewFileInfo = (FileInfo*)malloc(FileInfoLen);
				//copy path
				lstrcpy(pNewFileInfo->RelativeFilePath, pFile->RelativeFilePath);
				lstrcat(pNewFileInfo->RelativeFilePath, L"\\");
				lstrcat(pNewFileInfo->RelativeFilePath, fd.cFileName);

				pNewFileInfo->Attribute = fd.dwFileAttributes;
				pNewFileInfo->dwFileLengthHi = (fd.nFileSizeHigh);
				pNewFileInfo->dwFileLengthLo = fd.nFileSizeLow;
				//
				IterQueue.AddTail(pNewFileInfo);
			}
			//Find Next file;
			bNext = FindNextFileW(hFirst, &fd);
		}
		FindClose(hFirst);
		free(StartDir);
	}
}

