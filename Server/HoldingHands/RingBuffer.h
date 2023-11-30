#pragma once
class CRingBuffer
{
private:
	BYTE * m_buff;
	UINT32 m_size;
	UINT32 m_head;

public:
	int Read(BYTE *lpBuf, UINT Size);
	int Write(BYTE *lpBuf, UINT Size);


	CRingBuffer();
	~CRingBuffer();
};

