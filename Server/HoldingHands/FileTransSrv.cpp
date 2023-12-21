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

//����ֻ�з������Ż����.
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
	//ֻ�з������Ż����յ������Ϣ.
	case MNFT_INIT:
		OnMINIInit(Size, lpData);
		break;
	/*******************************************************************************/
		//when we Get File from peer.
	case MNFT_TRANS_INFO_RPL:
		OnGetTransInfoRpl(Size, (char*)lpData);							//��ȡ���˴�����Ϣ
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
	case MNFT_TRANS_INFO_GET:					//��ȡ������Ϣ(�ļ�����,�ܴ�С)
		OnGetTransInfo();
		break;
	case MNFT_FILE_INFO_GET:					//�Է���ʼ�����ļ��ˡ�
		OnGetFileInfo(Size, (char*)lpData);
		break;
	case MNFT_FILE_DATA_CHUNK_GET:				//��ȡ�ļ�����
		OnGetFileDataChunk(Size, (char*)lpData);
		break;
	case MNFT_FILE_TRANS_FINISHED:				//��ǰ�ļ��������(���ܳɹ�,Ҳ����ʧ��)
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
//�ļ����ݿ鵽����
void CFileTransSrv::OnGetFileDataChunkRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Data_Chunk_Data*pRpl = (MNFT_File_Data_Chunk_Data*)Buffer;
	if (pRpl->dwFileIdentity != 
		m_dwCurFileIdentity){
		Close();
		return;
	}
	//д���ļ�����
	DWORD dwWrite = 0;
	BOOL bFailed = (pRpl->dwChunkSize == 0 ||
		(!WriteFile(m_hCurFile, pRpl->FileDataChunk, pRpl->dwChunkSize, &dwWrite, NULL)) || 
		(dwWrite == 0));

	if (!bFailed)
	{
		//д��ɹ�
		m_ullLeftFileLength -= dwWrite;
		if (m_ullLeftFileLength)
		{
			//��������
			MNFT_File_Data_Chunk_Data_Get fdcdg;
			fdcdg.dwFileIdentity = m_dwCurFileIdentity;
			Send(MNFT_FILE_DATA_CHUNK_GET, &fdcdg, sizeof(fdcdg));
		}
		//����
		/*****************************************************************************/
		//��������ȥ����
		Notify(WM_MNFT_FILE_DC_TRANSFERRED, dwWrite, 0);
		/*****************************************************************************/
	}
	
	//���ʧ����,���߽�������.��ô�ͽ���ֹ��ǰ�Ĵ���,������һ���ļ�.
	if (bFailed || !m_ullLeftFileLength)
	{
		//�رվ��
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		//������ǰ�ļ��Ĵ���.
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = bFailed ? MNFT_STATU_FAILED : MNFT_STATU_SUCCESS;
		Send(MNFT_FILE_TRANS_FINISHED, &ftf, sizeof(ftf));
		/*****************************************************************************/
		//��������ȥ����
		Notify(WM_MNFT_FILE_TRANS_FINISHED, ftf.dwStatu, 0);
		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//������һ��
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

	////��������ȥ����
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
				//��ȡ���ݿ�
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

	//�ļ�����Ϊ0,���ߵ�ǰ�����ʧ��,��ô��������һ���ļ�.
	if (pFileInfo->Attribute & FILE_ATTRIBUTE_DIRECTORY || 
		m_ullLeftFileLength == 0 ||
		m_hCurFile == INVALID_HANDLE_VALUE)
	{
		
		//������ǰ�ļ��Ĵ���.
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
		//������һ��
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


//--------------------------------------������Щ����Ϊ���Ͷ�Ҫ��������------------------------------------------
//������ǰ�ļ��ķ���.
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
		//�������.�Ͽ�
		Close();
		return;
	}
	//��������,64kb�Ļ�����.
	MNFT_File_Data_Chunk_Data*pRpl = (MNFT_File_Data_Chunk_Data*)malloc(sizeof(DWORD) * 2 + 0x10000);
	pRpl->dwFileIdentity = m_dwCurFileIdentity;
	pRpl->dwChunkSize = 0;
	//�����ȡ������,Ҳ�᷵��TRUE,
	ReadFile(m_hCurFile, pRpl->FileDataChunk, 0x10000, &pRpl->dwChunkSize, NULL);
	//���˶��پͷ��Ͷ��١���ȡʧ�ܾ���0,�öԷ�  ������ǰ�ļ��Ķ�ȡ.
	Send(MNFT_FILE_DATA_CHUNK_RPL, (char*)pRpl, sizeof(DWORD) * 2 + pRpl->dwChunkSize);
	/**********************************************************************/
	Notify(WM_MNFT_FILE_DC_TRANSFERRED, pRpl->dwChunkSize, 0);
	/**********************************************************************/
	free(pRpl);
}

//�Է������ļ���
void CFileTransSrv::OnGetFileInfo(DWORD Read, char*Buffer)
{
	MNFT_File_Info_Get *pfig = (MNFT_File_Info_Get*)Buffer;
	if (!m_JobList.GetCount()){
		Send(MNFT_TRANS_FINISHED, 0, 0);//�������.
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
	//��������ȥ����
	Notify(WM_MNFT_FILE_TRANS_BEGIN, (WPARAM)&pReply->fiFileInfo, 0);

	/*************************************************************************/
	Send(MNFT_FILE_INFO_RPL, pReply, dwRplLen);
	free(pReply);
	//�������������Ĵ����ǲ���ִ�е�.��Ϊ��ʼ�����ÿ�ν������ͺ󶼻����
	if (m_hCurFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCurFile);
		m_hCurFile = INVALID_HANDLE_VALUE;
	}

	//�������Ŀ¼,��ô�򿪶�Ӧ���ļ� 
	if (!(m_JobList.GetHead()->Attribute & FILE_ATTRIBUTE_DIRECTORY))
	{
		DWORD dwFullPathLen = lstrlen(m_SrcDir) + 1 + lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1;
		TCHAR* FullPath = (TCHAR*)malloc(dwFullPathLen*sizeof(TCHAR));
		lstrcpy(FullPath, m_SrcDir);
		lstrcat(FullPath, TEXT("\\"));
		lstrcat(FullPath, m_JobList.GetHead()->RelativeFilePath);
		//���ļ����
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


//�Է���������Ϣ��,������Ϊ���ͷ�
void CFileTransSrv::OnGetTransInfo()
{
	BFS_GetFileList(m_SrcDir, m_FileList, &m_JobList);
	//	
	//�ظ�TransInfo
	MNFT_Trans_Info TransInfoRpl;
	TransInfoRpl.dwFileCount = m_JobList.GetCount();
	TransInfoRpl.TotalSizeLo = m_JobList.GetTotalSize()&0xFFFFFFFF;
	TransInfoRpl.TotalSizeHi = (m_JobList.GetTotalSize() & 0xFFFFFFFF00000000) >> 32;
	//
	Notify(WM_MNFT_TRANS_INFO, (WPARAM)&TransInfoRpl, m_Duty);
	Send(MNFT_TRANS_INFO_RPL, &TransInfoRpl, sizeof(MNFT_Trans_Info));
}

//��ȱ�����ȡ�ļ��б�.;
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
	//����\n
	while (szFileNameEnd[0] && szFileNameEnd[0] == L'\n')
		szFileNameEnd++;

	while (szFileNameEnd[0])
	{
		szFileName = szFileNameEnd;
		//�ҵ���β
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
		//NextFileName,����\n
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
