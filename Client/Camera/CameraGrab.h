#pragma once

#define STRSAFE_NO_DEPRECATE
#include <Windows.h>
#include <dshow.h>
#include "qedit.h"
#include "MySampleGrabberCB.h"
#include <map>
#include <set>
#include <string>
#include <list>
#include <stdint.h>
#include <iostream>

using std::set;
using std::string;
using std::map;
using std::list;
using std::pair;
using std::cout;
using std::endl;


extern "C"{
#include <x264\x264.h>
#include <libyuv\libyuv.h>
}



typedef map <string, set<pair<int, int>>> VideoInfoList;

//最大缓冲40MB,足够一张图片了.
#define MAX_BUF_SIZE (40*1024*1024)
class CCameraGrab
{
private:	

	friend class CMySampleGrabberCB;
public:

	DWORD m_dwHeight;
	DWORD m_dwWidth;
private:
	HANDLE	m_hEvent;									//标记FrameList 是否为空
	HANDLE	m_hMutex;

	list<pair<char*, DWORD>>	m_FrameList;

	//FrameList*		m_pFrameListHead;
	//FrameList*		m_pFrameListTail;

	DWORD			m_dwBufSize;

	void AddTail(char*, DWORD);
	void RemoveHead(char**ppBuffer, DWORD*pLen);

	IGraphBuilder		  *m_pGraph;
	ICaptureGraphBuilder2 *m_pBuild;
	IBaseFilter			  *m_pCaptureFilter;
	IBaseFilter			  *m_pSampleGrabberFilter;
	ISampleGrabber		  *m_pSamplerGrabber;
	IBaseFilter		      *m_pNullRenderer;
	IMediaControl		  *m_pMediaControl;
	CMySampleGrabberCB	  *m_pCallback;

	//x264
	x264_t*	m_pEncoder;		//编码器实例
	x264_picture_t *m_pPicIn;
	x264_picture_t *m_pPicOut;

	BOOL	m_IsGrabbing;
	//摄像头信息
	VideoInfoList m_device;
	void GetDeviceList(const string& device_name, IBaseFilter**ppBaseFilter);

	void _FreeMediaType(AM_MEDIA_TYPE& mt);
	void _DeleteMediaType(AM_MEDIA_TYPE *pmt);

public:
	VideoInfoList& GetDeviceList();

	int GrabberInit(const string&device_name, DWORD dwWidth, DWORD dwHeightm, char*szError);
	BOOL StartGrab();
	void StopGrab();
	BOOL GetFrame(BYTE**lppbuffer, UINT32 * plen);

	void GrabberTerm();
	CCameraGrab();
	~CCameraGrab();
};

