#include "stdafx.h"
#include "FileTransSrv.h"

CFileTransSrv::CFileTransSrv(CClient*pClient) :
CEventHandler(pClient, MINIFILETRANS),
	m_dwCurFileIdentity(0),
	m_hCurFile(INVALID_HANDLE_VALUE),
	m_ullLeftFileLength(0),
	m_dwCurFileAttribute(0),
	m_TransferFinished(FALSE)
{
	m_SrcDir = NULL;
	m_DestDir = NULL;
	m_FileList = NULL;
}


CFileTransSrv::~CFileTransSrv()
{
	delete[] m_SrcDir;
	delete[] m_DestDir;
	delete[] m_FileList;
}



void CFileTransSrv::OnClose()
{
	Clean();
}

void CFileTransSrv::OnOpen()
{
}


void CFileTransSrv::OnTransferFinished()
{
	m_TransferFinished = TRUE;
	Notify(WM_MNFT_TRANS_FINISHED);
}

//这里只有服务器才会调用.
void CFileTransSrv::OnMINIInit(DWORD Read, BYTE * init)
{
	memcpy(&m_Duty, init,sizeof(UINT32));
	init += sizeof(UINT32);

	m_SrcDir = new TCHAR[lstrlen((TCHAR*)init) + 1];
	lstrcpy(m_SrcDir, (TCHAR*)init);
	init += sizeof(TCHAR) * (lstrlen((TCHAR*)init) + 1);


	m_DestDir = new TCHAR[lstrlen((TCHAR*)init) + 1];
	lstrcpy(m_DestDir, (TCHAR*)init);
	init += sizeof(TCHAR) * (lstrlen((TCHAR*)init) + 1);

	m_FileList = new TCHAR[lstrlen((TCHAR*)init) + 1];
	lstrcpy(m_FileList, (TCHAR*)init);
	init += sizeof(TCHAR) * (lstrlen((TCHAR*)init) + 1);


	if (m_Duty == MNFT_DUTY_RECEIVER)
	{
		Send(MNFT_TRANS_INFO_GET, NULL, NULL);
	}
}



void CFileTransSrv::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
	/*******************************************************************************/
	//只有服务器才会受收到这个消息.
	case MNFT_INIT:
		OnMINIInit(Size, lpData);
		break;
	/*******************************************************************************/
		//when we Get File from peer.
	case MNFT_TRANS_INFO_RPL:
		OnGetTransInfoRpl(Size, (char*)lpData);							//获取到了传输信息
		break;
	case MNFT_FILE_INFO_RPL:
		OnGetFileInfoRpl(Size, (char*)lpData);
		break;
	case MNFT_FILE_DATA_CHUNK_RPL:
		OnGetFileDataChunkRpl(Size, (char*)lpData);
		break;
	case MNFT_TRANS_FINISHED:
		OnTransferFinished();
		break;
	/*******************************************************************************/
		//when we Send File to peer.
	case MNFT_TRANS_INFO_GET:					//获取传输信息(文件个数,总大小)
		OnGetTransInfo();
		break;
	case MNFT_FILE_INFO_GET:					//对方开始请求文件了。
		OnGetFileInfo(Size, (char*)lpData);
		break;
	case MNFT_FILE_DATA_CHUNK_GET:				//获取文件数据
		OnGetFileDataChunk(Size, (char*)lpData);
		break;
	case MNFT_FILE_TRANS_FINISHED:				//当前文件传输完成(可能成功,也可能失败)
		OnFileTransFinished(Size, (char*)lpData);
		break;
	}
}


void CFileTransSrv::Clean()
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
void CFileTransSrv::OnGetFileDataChunkRpl(DWORD Read, char*Buffer)
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
			Send(MNFT_FILE_DATA_CHUNK_GET, &fdcdg, sizeof(fdcdg));
		}
		//接收
		/*****************************************************************************/
		//交给窗口去处理
		Notify(WM_MNFT_FILE_DC_TRANSFERRED, dwWrite, 0);
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
		Send(MNFT_FILE_TRANS_FINISHED, &ftf, sizeof(ftf));
		/*****************************************************************************/
		//交给窗口去处理
		Notify(WM_MNFT_FILE_TRANS_FINISHED, ftf.dwStatu, 0);
		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//传输下一个
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		Send(MNFT_FILE_INFO_GET, &fig, sizeof(fig));
	}
}

void CFileTransSrv::OnGetFileInfoRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Info *pRpl = (MNFT_File_Info*)Buffer;
	
	if (pRpl->dwFileIdentity != m_dwCurFileIdentity)
	{
		Close();
		return;
	}
	//
	FileInfo*pFileInfo = &pRpl->fiFileInfo;

	m_dwCurFileAttribute = pFileInfo->Attribute;
	m_ullLeftFileLength = pFileInfo->dwFileLengthHi;
	m_ullLeftFileLength = (m_ullLeftFileLength << 32) + pFileInfo->dwFileLengthLo;
	//
	DWORD FullPathLen = lstrlen(m_DestDir) + 1 + lstrlen(pFileInfo->RelativeFilePath) + 1;
	TCHAR* FullPath = (TCHAR*)malloc(FullPathLen*sizeof(TCHAR));
	lstrcpy(FullPath, m_DestDir);
	lstrcat(FullPath, TEXT("\\"));
	lstrcat(FullPath, pFileInfo->RelativeFilePath);

	////交给窗口去处理
	Notify(WM_MNFT_FILE_TRANS_BEGIN, (WPARAM)pFileInfo, 0);

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
			m_hCurFile = CreateFile(
				FullPath, 
				GENERIC_WRITE,
				0,
				NULL, 
				CREATE_ALWAYS, 
				m_dwCurFileAttribute,
				NULL);
			
			//GetFileChunk.
			if (m_hCurFile != INVALID_HANDLE_VALUE)
			{
				//获取数据块
				MNFT_File_Data_Chunk_Data_Get fdcdg;
				fdcdg.dwFileIdentity = m_dwCurFileIdentity;
				Send(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
			}
		}
	}
	else
	{
		CreateDirectory(FullPath, NULL);		//MakesureDirectoryExist
	}

	//文件长度为0,或者当前句柄打开失败,那么就请求下一个文件.
	if (pFileInfo->Attribute & FILE_ATTRIBUTE_DIRECTORY || 
		m_ullLeftFileLength == 0 ||
		m_hCurFile == INVALID_HANDLE_VALUE)
	{
		
		//结束当前文件的传输.
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY 
			|| m_ullLeftFileLength == 0) ? MNFT_STATU_SUCCESS : MNFT_STATU_FAILED;

		Send(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		//
		/*****************************************************************************/
		Notify(WM_MNFT_FILE_TRANS_FINISHED, ftf.dwStatu, 0);

		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//传输下一个
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		Send(MNFT_FILE_INFO_GET, &fig, sizeof(fig));
	}
	free(FullPath);
}

void CFileTransSrv::OnGetTransInfoRpl(DWORD Read, char*Buffer)
{
	MNFT_Trans_Info*pTransInfo = (MNFT_Trans_Info*)Buffer;

	//Save trans info.
	/************************************************************************/
	Notify(WM_MNFT_TRANS_INFO, (WPARAM)pTransInfo, m_Duty);

	/************************************************************************/
	//Get First File,use timestamp as the file identity.
	MNFT_File_Info_Get fig;
	m_dwCurFileIdentity = GetTickCount();
	fig.dwFileIdentity = m_dwCurFileIdentity;
	Send(MNFT_FILE_INFO_GET, &fig, sizeof(fig));
}


//--------------------------------------下面这些是作为发送端要做的事情------------------------------------------
//结束当前文件的发送.
void CFileTransSrv::OnFileTransFinished(DWORD Read, char*Buffer)
{
	MNFT_File_Trans_Finished*pftf = (MNFT_File_Trans_Finished*)Buffer;
	
	if (pftf->dwFileIdentity != m_dwCurFileIdentity)
	{
		Close();
		return;
	}
	
	if (m_hCurFile != INVALID_HANDLE_VALUE)
	{
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
	Notify(WM_MNFT_FILE_TRANS_FINISHED, pftf->dwStatu, 0);

	/***********************************************************************************/
}
void CFileTransSrv::OnGetFileDataChunk(DWORD Read, char*Buffer)
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
	Send(MNFT_FILE_DATA_CHUNK_RPL, (char*)pRpl, sizeof(DWORD) * 2 + pRpl->dwChunkSize);
	/**********************************************************************/
	Notify(WM_MNFT_FILE_DC_TRANSFERRED, pRpl->dwChunkSize, 0);
	/**********************************************************************/
	free(pRpl);
}

//对方请求文件了
void CFileTransSrv::OnGetFileInfo(DWORD Read, char*Buffer)
{
	MNFT_File_Info_Get *pfig = (MNFT_File_Info_Get*)Buffer;
	if (!m_JobList.GetCount()){
		Send(MNFT_TRANS_FINISHED, 0, 0);//传输结束.
		OnTransferFinished();
		return;
	}

	//save file identity;
	m_dwCurFileIdentity = pfig->dwFileIdentity;
	//
	DWORD dwRplLen = 
		sizeof(DWORD) * 4 +
		sizeof(TCHAR)* (lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1);

	MNFT_File_Info*pReply = (MNFT_File_Info*)malloc(dwRplLen);

	pReply->dwFileIdentity = m_dwCurFileIdentity;
	pReply->fiFileInfo.Attribute = m_JobList.GetHead()->Attribute;
	pReply->fiFileInfo.dwFileLengthLo = m_JobList.GetHead()->dwFileLengthLo;
	pReply->fiFileInfo.dwFileLengthHi = m_JobList.GetHead()->dwFileLengthHi;

	lstrcpy(pReply->fiFileInfo.RelativeFilePath, m_JobList.GetHead()->RelativeFilePath);
	/*************************************************************************/
	//交给窗口去处理
	Notify(WM_MNFT_FILE_TRANS_BEGIN, (WPARAM)&pReply->fiFileInfo, 0);

	/*************************************************************************/
	Send(MNFT_FILE_INFO_RPL, pReply, dwRplLen);
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
		DWORD dwFullPathLen = lstrlen(m_SrcDir) + 1 + lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1;
		TCHAR* FullPath = (TCHAR*)malloc(dwFullPathLen*sizeof(TCHAR));
		lstrcpy(FullPath, m_SrcDir);
		lstrcat(FullPath, TEXT("\\"));
		lstrcat(FullPath, m_JobList.GetHead()->RelativeFilePath);
		//打开文件句柄
		m_hCurFile = CreateFile(
			FullPath, 
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL, 
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			0);
		//
		free(FullPath);
	}

}


//对方请求传输信息了,本地作为发送方
void CFileTransSrv::OnGetTransInfo()
{
	BFS_GetFileList(m_SrcDir, m_FileList, &m_JobList);
	//	
	//回复TransInfo
	MNFT_Trans_Info TransInfoRpl;
	TransInfoRpl.dwFileCount = m_JobList.GetCount();
	TransInfoRpl.TotalSizeLo = m_JobList.GetTotalSize()&0xFFFFFFFF;
	TransInfoRpl.TotalSizeHi = (m_JobList.GetTotalSize() & 0xFFFFFFFF00000000) >> 32;
	//
	Notify(WM_MNFT_TRANS_INFO, (WPARAM)&TransInfoRpl, m_Duty);
	Send(MNFT_TRANS_INFO_RPL, &TransInfoRpl, sizeof(MNFT_Trans_Info));
}

//广度遍历获取文件列表.;
void CFileTransSrv::BFS_GetFileList(TCHAR*Path, TCHAR*FileNameList, FileInfoList*pFileInfoList)
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
		DWORD dwNameLen = ((DWORD)szFileNameEnd - (DWORD)szFileName) / sizeof(TCHAR);

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
		lstrcat(FullPath, TEXT("\\"));
		lstrcat(FullPath, pNewFileInfo->RelativeFilePath);

		WIN32_FIND_DATA fd;
		HANDLE hFirst = FindFirstFile(FullPath, &fd);

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
		lstrcat(StartDir, TEXT("\\"));
		lstrcat(StartDir, (pFile->RelativeFilePath));
		lstrcat(StartDir, TEXT("\\*"));

		//path\\filepath
		WIN32_FIND_DATA fd = { 0 };
		HANDLE hFirst = FindFirstFile(StartDir, &fd);
		BOOL bNext = TRUE;
		//trave this dir:
		while (hFirst != INVALID_HANDLE_VALUE && bNext)
		{
			if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
				(lstrcmp(fd.cFileName, TEXT(".")) && lstrcmp(fd.cFileName, TEXT(".."))))
			{

				//allocate path
				DWORD NewPathLen = lstrlen(pFile->RelativeFilePath) + 1 + lstrlen(fd.cFileName) + 1;

				DWORD FileInfoLen = 3 * sizeof(DWORD) + NewPathLen * sizeof(TCHAR);

				FileInfo*pNewFileInfo = (FileInfo*)malloc(FileInfoLen);
				//copy path
				lstrcpy(pNewFileInfo->RelativeFilePath, pFile->RelativeFilePath);
				lstrcat(pNewFileInfo->RelativeFilePath, TEXT("\\"));
				lstrcat(pNewFileInfo->RelativeFilePath, fd.cFileName);

				pNewFileInfo->Attribute = fd.dwFileAttributes;
				pNewFileInfo->dwFileLengthHi = (fd.nFileSizeHigh);
				pNewFileInfo->dwFileLengthLo = fd.nFileSizeLow;
				//
				IterQueue.AddTail(pNewFileInfo);
			}
			//Find Next file;
			bNext = FindNextFile(hFirst, &fd);
		}
		FindClose(hFirst);
		free(StartDir);
	}
}
