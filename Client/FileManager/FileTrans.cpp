#include "FileTrans.h"
#include "utils.h"

BOOL MakesureDirExist(const TCHAR* Path,BOOL bIncludeFileName = FALSE);


CFileTrans::CFileTrans(CClient*pClient, UINT32 Duty, const TCHAR * SrcDir, const TCHAR* DestDir, const TCHAR * FileList) :
CEventHandler(pClient, MINIFILETRANS)
{
	m_dwCurFileIdentity = 0;
	m_hCurFile = INVALID_HANDLE_VALUE;
	m_ullLeftFileLength = 0;			//ʣ�೤��;
	m_dwCurFileAttribute = 0;			//��ǰ�ļ�����;

	m_Duty = Duty;

	m_SrcDir = (TCHAR*)malloc(sizeof(TCHAR) * (lstrlen(SrcDir) + 1));
	lstrcpy(m_SrcDir, SrcDir);

	m_DestDir = (TCHAR*)malloc(sizeof(TCHAR) * (lstrlen(DestDir) + 1));
	lstrcpy(m_DestDir, DestDir);

	m_FileList = (TCHAR*)malloc(sizeof(TCHAR) * (lstrlen(FileList) + 1));
	lstrcpy(m_FileList, FileList);

}


CFileTrans::~CFileTrans()
{
	if (m_SrcDir)
	{
		free(m_SrcDir);
		m_SrcDir = NULL;
	}

	if (m_DestDir)
	{
		free(m_DestDir);
		m_DestDir = NULL;
	}

	if (m_FileList)
	{
		free(m_FileList);
		m_FileList = NULL;
	}
}

void CFileTrans::OnClose()
{
	Clean();
}


void CFileTrans::OnOpen()
{
	//
	TCHAR* pSavePath = NULL;
	vec bufs[4] = { 0 };
	UINT32 duty = ~m_Duty;

	bufs[0].lpData = &duty;
	bufs[0].Size = sizeof(duty);

	bufs[1].lpData = m_SrcDir;
	bufs[1].Size = sizeof(TCHAR) * (lstrlen(m_SrcDir) + 1);

	bufs[2].lpData = m_DestDir;
	bufs[2].Size = sizeof(TCHAR) * (lstrlen(m_DestDir) + 1);

	bufs[3].lpData = m_FileList;
	bufs[3].Size = sizeof(TCHAR) * (lstrlen(m_FileList) + 1);

	Send(MNFT_INIT, bufs, 4);

	if (m_Duty == MNFT_DUTY_RECEIVER)
	{
		//send request to client.
		Send(MNFT_TRANS_INFO_GET,NULL,NULL);//����\n֮�������.;
	}
	else
	{
		//ʲôҲ������,�ȴ��Է������ļ�.;
	}
}

void CFileTrans::OnEvent(UINT32 e, BYTE *lpData, UINT32 Size)
{
	switch (e)
	{
		/*******************************************************************************/
		/*******************************************************************************/
		//when we Get File from peer.
	case MNFT_TRANS_INFO_RPL:
		OnGetTransInfoRpl(Size, (char*)lpData);			//��ȡ���˴�����Ϣ;
		break;
	case MNFT_FILE_INFO_RPL:
		OnGetFileInfoRpl(Size, (char*)lpData);
		break;
	case MNFT_FILE_DATA_CHUNK_RPL:
		OnGetFileDataChunkRpl(Size, (char*)lpData);
		break;
	case MNFT_TRANS_FINISHED:
		OnTransFinished();						//�������;
		break;
		/*******************************************************************************/
		//when we Send File to peer.
	case MNFT_TRANS_INFO_GET:					//��ȡ������Ϣ(�ļ�����,�ܴ�С);
		OnGetTransInfo();
		break;
	case MNFT_FILE_INFO_GET:					//�Է���ʼ�����ļ��ˡ�;
		OnGetFileInfo(Size, (char*)lpData);
		break;
	case MNFT_FILE_DATA_CHUNK_GET:				//��ȡ�ļ�����;
		OnGetFileDataChunk(Size, (char*)lpData);
		break;
	case MNFT_FILE_TRANS_FINISHED:				//��ǰ�ļ��������(���ܳɹ�,Ҳ����ʧ��);
		OnFileTransFinished(Size, (char*)lpData);
		break;
	}
}

void CFileTrans::OnTransFinished(){
	Close();
}

void CFileTrans::Clean()
{
	while (m_JobList.GetCount())
	{
		free(m_JobList.RemoveHead());
	}
	
	if (m_hCurFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCurFile);
		m_hCurFile = INVALID_HANDLE_VALUE;
	}
	
	m_dwCurFileAttribute = 0;
	m_dwCurFileIdentity = 0;
	m_ullLeftFileLength = 0;
}
//�ļ����ݿ鵽����;
void CFileTrans::OnGetFileDataChunkRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Data_Chunk_Data_Rpl*pRpl = (MNFT_File_Data_Chunk_Data_Rpl*)Buffer;
	if (pRpl->dwFileIdentity != m_dwCurFileIdentity){
		Close();
		return;
	}
	//д���ļ�����;
	DWORD dwWrite = 0;
	BOOL bFailed = (pRpl->dwChunkSize == 0 ||
		(!WriteFile(m_hCurFile, pRpl->FileDataChunk, pRpl->dwChunkSize, &dwWrite, NULL)) || 
		(dwWrite == 0));
	if (!bFailed)
	{
		//д��ɹ�;
		m_ullLeftFileLength -= dwWrite;
		if (m_ullLeftFileLength)
		{
			//��������;
			MNFT_File_Data_Chunk_Data_Get fdcdg;
			fdcdg.dwFileIdentity = m_dwCurFileIdentity;
			Send(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
		}
		//����;
	}
	//���ʧ����,���߽�������.��ô�ͽ���ֹ��ǰ�Ĵ���,������һ���ļ�.;
	if (bFailed || !m_ullLeftFileLength)
	{
		//�رվ��;
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		//������ǰ�ļ��Ĵ���.;
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = bFailed ? MNFT_STATU_FAILED : MNFT_STATU_SUCCESS;
		Send(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		/*****************************************************************************/
		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//������һ��;
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		Send(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
	}
}

void CFileTrans::OnGetFileInfoRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Info_Rpl *pRpl = (MNFT_File_Info_Rpl*)Buffer;
	if (pRpl->dwFileIdentity != m_dwCurFileIdentity){
		Close();
		return;
	}
	//
	FileInfo*pFileInfo = &pRpl->fiFileInfo;

	m_dwCurFileAttribute = pFileInfo->Attribute;
	m_ullLeftFileLength = pFileInfo->dwFileLengthHi;
	m_ullLeftFileLength<<=32;
	m_ullLeftFileLength |= pFileInfo->dwFileLengthLo;
	//
	DWORD FullPathLen = lstrlen(m_DestDir) + 1 + lstrlen(pFileInfo->RelativeFilePath) + 1;
	PTCHAR FullPath = (TCHAR*)malloc(FullPathLen * sizeof(TCHAR));
	
	lstrcpy(FullPath, m_DestDir);
	lstrcat(FullPath, TEXT("\\"));
	lstrcat(FullPath, pFileInfo->RelativeFilePath);
	//
	if (!(pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY))
	{
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		if (m_ullLeftFileLength != 0)
		{
			//
			MakesureDirExist(FullPath,TRUE);
			//CreateFile.
			m_hCurFile = CreateFile(FullPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, m_dwCurFileAttribute, NULL);
			//GetFileChunk.;
			if (m_hCurFile != INVALID_HANDLE_VALUE)
			{
				//��ȡ���ݿ�;
				MNFT_File_Data_Chunk_Data_Get fdcdg;
				fdcdg.dwFileIdentity = m_dwCurFileIdentity;
				Send(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
			}
		}
	}
	else
	{
		MakesureDirExist(FullPath);
	}
	//�ļ�����Ϊ0,���ߵ�ǰ�����ʧ��,��ô��������һ���ļ�.;
	if (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY || m_ullLeftFileLength == 0 || m_hCurFile == INVALID_HANDLE_VALUE)
	{

		//������ǰ�ļ��Ĵ���.;
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY || m_ullLeftFileLength == 0) ? MNFT_STATU_SUCCESS : MNFT_STATU_FAILED;
		Send(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		//
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//������һ��;
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		Send(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
	}
	free(FullPath);
}

void CFileTrans::OnGetTransInfoRpl(DWORD Read, char*Buffer)
{
	/************************************************************************/
	//Get First File,use timestamp as the file identity.
	MNFT_File_Info_Get fig;
	m_dwCurFileIdentity = GetTickCount();
	fig.dwFileIdentity = m_dwCurFileIdentity;
	Send(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
}


//--------------------------------------������Щ����Ϊ���Ͷ�Ҫ��������------------------------------------------;
//������ǰ�ļ��ķ���.;
void CFileTrans::OnFileTransFinished(DWORD Read, char*Buffer)
{
	MNFT_File_Trans_Finished*pftf = (MNFT_File_Trans_Finished*)Buffer;
	if (pftf->dwFileIdentity != m_dwCurFileIdentity){
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
}
void CFileTrans::OnGetFileDataChunk(DWORD Read, char*Buffer)
{
	if (((MNFT_File_Data_Chunk_Data_Get*)Buffer)->dwFileIdentity != m_dwCurFileIdentity)
	{
		//�������.�Ͽ�;
		Close();
		return;
	}
	//��������;
	MNFT_File_Data_Chunk_Data_Rpl*pRpl = (MNFT_File_Data_Chunk_Data_Rpl*)malloc(sizeof(DWORD) * 2 + 0x10000);
	pRpl->dwFileIdentity = m_dwCurFileIdentity;
	pRpl->dwChunkSize = 0;
	//�����ȡ������,Ҳ�᷵��TRUE,;
	ReadFile(m_hCurFile, pRpl->FileDataChunk, 0x10000, &pRpl->dwChunkSize, NULL);
	//���˶��پͷ��Ͷ��١���ȡʧ�ܾ���0,�öԷ�  ������ǰ�ļ��Ķ�ȡ.;
	Send(MNFT_FILE_DATA_CHUNK_RPL, (char*)pRpl, sizeof(DWORD) * 2 + pRpl->dwChunkSize);
	free(pRpl);
}

//�Է������ļ���;
void CFileTrans::OnGetFileInfo(DWORD Read, char*Buffer)
{
	MNFT_File_Info_Get *pfig = (MNFT_File_Info_Get*)Buffer;
	if (!m_JobList.GetCount())
	{
		Send(MNFT_TRANS_FINISHED, 0, 0);//�������.;
		Close();
	}
	else
	{
		//save file identity;
		m_dwCurFileIdentity = pfig->dwFileIdentity;
		//
		DWORD dwRplLen = 
			sizeof(DWORD) * 4 +
			sizeof(TCHAR)* (lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1);

		MNFT_File_Info_Rpl*pReply = (MNFT_File_Info_Rpl*)malloc(dwRplLen);

		pReply->dwFileIdentity = m_dwCurFileIdentity;
		pReply->fiFileInfo.Attribute = m_JobList.GetHead()->Attribute;
		pReply->fiFileInfo.dwFileLengthHi = m_JobList.GetHead()->dwFileLengthHi;
		pReply->fiFileInfo.dwFileLengthLo = m_JobList.GetHead()->dwFileLengthLo;

		lstrcpy(pReply->fiFileInfo.RelativeFilePath, m_JobList.GetHead()->RelativeFilePath);
		Send(MNFT_FILE_INFO_RPL, (char*)pReply, dwRplLen);
		free(pReply);
		//�������������Ĵ����ǲ���ִ�е�.��Ϊ��ʼ�����ÿ�ν������ͺ󶼻����;
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		//�������Ŀ¼,��ô�򿪶�Ӧ���ļ� ;
		if (!(m_JobList.GetHead()->Attribute & FILE_ATTRIBUTE_DIRECTORY))
		{
			DWORD dwFullPathLen = lstrlen(m_SrcDir) + 1 + lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1;
			PTCHAR FullPath = (TCHAR*)malloc(dwFullPathLen*sizeof(TCHAR));
			lstrcpy(FullPath, m_SrcDir);
			lstrcat(FullPath, TEXT("\\"));
			lstrcat(FullPath, m_JobList.GetHead()->RelativeFilePath);
			//���ļ����;
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
}


//�Է���������Ϣ��,������Ϊ���ͷ�;
void CFileTrans::OnGetTransInfo()
{
	BFS_GetFileList(m_SrcDir, m_FileList, &m_JobList);
	//
	//�ظ�TransInfo;
	MNFT_Trans_Info_Rpy TransInfoRpl;
	TransInfoRpl.dwFileCount =  m_JobList.GetCount();
	TransInfoRpl.TotalSizeLo = m_JobList.GetTotalSize()&0xffffffff;
	TransInfoRpl.TotalSizeHi = (m_JobList.GetTotalSize()&0xffffffff00000000)>>32;
	Send(MNFT_TRANS_INFO_RPL, &TransInfoRpl, sizeof(MNFT_Trans_Info_Rpy));
}

//��ȱ�����ȡ�ļ��б�.;
void CFileTrans::BFS_GetFileList(TCHAR*Path, TCHAR*FileNameList, FileInfoList*pFileInfoList)
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

				DWORD FileInfoLen = 3*sizeof(DWORD) + NewPathLen * sizeof(TCHAR);

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
