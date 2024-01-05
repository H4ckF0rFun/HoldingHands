#pragma once
#ifndef _MINI_FILE_TRANS_SVR
#define _MINI_FILE_TRANS_SVR

#include "file_transfer_common.h"
#include "EventHandler.h"
#include "module.h"
/************************************************************************************/


class CManager;

class CFileTrans :
	public CEventHandler
{
private:

	//some structures will be used when we thansfer files;
	struct FileInfo
	{
		DWORD	  dwFileLengthHi;
		DWORD	  dwFileLengthLo;
		DWORD	  Attribute;
		TCHAR	  RelativeFilePath[2];			//2是为了对齐.
	};
	/***************************************************************************/

	struct MNFT_Trans_Info_Rpy
	{
		DWORD		TotalSizeHi;		//文件总大小;
		DWORD		TotalSizeLo;		//文件总大小;
		DWORD		dwFileCount;		//文件个数;
	};
	/***************************************************************************/
	struct MNFT_File_Info_Rpl
	{
		DWORD		dwFileIdentity;
		FileInfo	fiFileInfo;
	};

	struct MNFT_File_Info_Get
	{
		DWORD		dwFileIdentity;
	};
	/***************************************************************************/

	struct MNFT_File_Data_Chunk_Data_Get
	{
		DWORD		dwFileIdentity;
	};

	struct MNFT_File_Data_Chunk_Data_Rpl
	{
		DWORD		dwFileIdentity;
		DWORD		dwChunkSize;
		BYTE		FileDataChunk[4];		//4是为了对齐.
	};
	/***************************************************************************/

	struct MNFT_File_Trans_Finished
	{
		DWORD		dwFileIdentity;		//
		DWORD		dwStatu;			//SUCCESS ,Failed
	};

	/***************************************************************************/

	class FileInfoList
	{
	private:
		struct Node
		{
			FileInfo*pFileInfo;
			Node*next;
		};
		ULONGLONG	m_ullTotalLength;
		DWORD		m_Count;
		Node*		m_pHead;
		Node*		m_pTail;
	public:
		FileInfoList(){
			m_ullTotalLength = 0;
			m_Count = 0;
			m_pHead = m_pTail = NULL;
		}
		~FileInfoList()
		{
			while (m_pHead){
				Node*pTemp = m_pHead;
				m_pHead = m_pHead->next;
				free(pTemp);
			}
		}
		void AddTail(FileInfo*pFileInfo){
			Node*pNewNode = (Node*)malloc(sizeof(Node));
			pNewNode->next = 0;
			pNewNode->pFileInfo = pFileInfo;
			
			ULONGLONG FileSize = pFileInfo->dwFileLengthHi;
			FileSize <<= 32;
			FileSize |= pFileInfo->dwFileLengthLo;

			m_ullTotalLength += FileSize;

			if (m_Count == 0){
				m_pHead = m_pTail = pNewNode;
			}
			else{
				m_pTail->next = pNewNode;
				m_pTail = pNewNode;
			}
			m_Count++;
		}
		FileInfo* RemoveHead(){
			if (!m_Count)
				return NULL;
			m_Count--;
			FileInfo*pResult = m_pHead->pFileInfo;

			ULONGLONG FileSize = pResult->dwFileLengthHi;
			FileSize <<= 32;
			FileSize |= pResult->dwFileLengthLo;

			m_ullTotalLength -= FileSize;
			Node*pTemp = m_pHead;
			if (m_pHead == m_pTail)
			{
				m_pHead = m_pTail = NULL;
			}
			else
			{
				m_pHead = m_pHead->next;
			}
			free(pTemp);
			return pResult;
		}
		FileInfo*GetHead(){
			if (m_pHead)
				return m_pHead->pFileInfo;
			return NULL;
		}
		FileInfo*GetTail(){
			if (m_pTail)
				return m_pTail->pFileInfo;
			return NULL;
		}
		DWORD GetCount(){
			return m_Count;
		}
		ULONGLONG GetTotalSize(){
			return m_ullTotalLength;
		}
	};


private:
	Module *        m_owner;


	//common
	DWORD			m_dwCurFileIdentity;
	HANDLE			m_hCurFile;
	//recv use
	ULONGLONG		m_ullLeftFileLength;
	DWORD			m_dwCurFileAttribute;
	//send use
	FileInfoList	m_JobList;

	//init info
	UINT32  m_Duty;
	TCHAR * m_SrcDir;
	TCHAR * m_DestDir;
	TCHAR * m_FileList;

private:

	void OnClose();	
	void OnOpen();

	void OnEvent(UINT32 e, BYTE *lpData, UINT32 Size);
	void Clean();
	//

	/***************************************************************************/
	//作为接收端;
	//获取文件列表;
	void BFS_GetFileList(TCHAR*Path, TCHAR*FileNameList, FileInfoList*pFileInfoList);
	void OnGetTransInfo();

	//获取文件信息
	void OnGetFileInfo(DWORD Read, char*Buffer);
	//获取文件数据.;
	void OnGetFileDataChunk(DWORD Read, char*Buffer);
	//结束当前文件的发送.;
	void OnFileTransFinished(DWORD Read, char*Buffer);
	//传输结束;
	void OnTransFinished();
	/***************************************************************************/
	
	//作为发送端.;
	void OnGetTransInfoRpl(DWORD Read, char*Buffer);
	void OnGetFileInfoRpl(DWORD Read, char*Buffer);
	void OnGetFileDataChunkRpl(DWORD Read, char*Buffer);

public:
	CFileTrans(CClient*pClient,Module * owner ,UINT32 Duty, const TCHAR * SrcDir, const TCHAR* DestDir, const TCHAR * FileList);
	~CFileTrans();
};

#endif