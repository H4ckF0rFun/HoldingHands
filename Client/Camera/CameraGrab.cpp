#include "CameraGrab.h"
#include <stdint.h>
#include <x264\x264.h>
#include "utils.h"
#include "dbg.h"

#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "libyuv.lib")
#pragma comment(lib,"libx264.lib")

CCameraGrab::CCameraGrab()
{
	m_dwHeight = 0;
	m_dwWidth = 0;

	m_hEvent = NULL;
	m_hMutex = NULL;

	m_pGraph = NULL;
	m_pBuild = NULL;
	m_pCaptureFilter = NULL;

	m_pSampleGrabberFilter = NULL;
	m_pSamplerGrabber = NULL;
	m_pNullRenderer = NULL;
	m_pMediaControl = NULL;

	m_pCallback = NULL;
	m_dwBufSize = NULL;

	m_pEncoder = NULL;
	m_pPicOut = NULL;
	m_pPicIn = NULL;

	m_IsGrabbing = FALSE;
}


CCameraGrab::~CCameraGrab()
{
	GrabberTerm();
}

void CCameraGrab::_FreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		// pUnk should not be used.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}


// Delete a media type structure that was allocated on the heap.
void CCameraGrab::_DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt != NULL)
	{
		_FreeMediaType(*pmt);
		CoTaskMemFree(pmt);
	}
}

void CCameraGrab::GrabberTerm()
{
	if (m_pMediaControl){
		m_pMediaControl->Stop();
		m_pMediaControl->Release();
		m_pMediaControl = NULL;
	}

	if (m_hMutex){
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}
	if (m_hEvent){
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	if (m_pGraph){
		m_pGraph->Release();
		m_pGraph = NULL;
	}
	if (m_pBuild){
		m_pBuild->Release();
		m_pBuild = NULL;
	}
	if (m_pCaptureFilter){
		m_pCaptureFilter->Release();
		m_pCaptureFilter = NULL;
	}

	if (m_pSamplerGrabber){
		m_pSamplerGrabber->Release();
		m_pSamplerGrabber = NULL;
	}

	if (m_pSampleGrabberFilter){
		m_pSampleGrabberFilter->Release();
		m_pSampleGrabberFilter = NULL;
	}
	if (m_pNullRenderer){
		m_pNullRenderer->Release();
		m_pNullRenderer = NULL;
	}
	if (m_pCallback){
		delete m_pCallback;
		m_pCallback = NULL;
	}

	while (m_FrameList.size()){
		auto& frame = m_FrameList.front();
		if (frame.first){
			delete[] frame.first;
		}
		m_FrameList.pop_front();
	}

	m_dwBufSize = NULL;
	//x264
	if (m_pEncoder){
		x264_encoder_close(m_pEncoder);
		m_pEncoder = NULL;
	}
	if (m_pPicIn){
		x264_picture_clean(m_pPicIn);
		free(m_pPicIn);
		m_pPicIn = NULL;
	}
	if (m_pPicOut){
		free(m_pPicOut);
		m_pPicOut = NULL;
	}
	m_dwHeight = 0;
	m_dwWidth = 0;

	m_IsGrabbing = FALSE;
	//printf("Grabber Term Ok\n");
}

void CCameraGrab::AddTail(char*buffer,DWORD dwLen)
{
	if (m_dwBufSize > MAX_BUF_SIZE)
		return;
	
	//Save Data...
	pair<char*, DWORD> frame(new char[dwLen],dwLen);

	m_dwBufSize += dwLen;
	memcpy(frame.first, buffer, dwLen);
	
	//list lock....
	WaitForSingleObject(m_hMutex, INFINITE);
	m_FrameList.push_back(frame);

	SetEvent(m_hEvent);
	SetEvent(m_hMutex);
}
void CCameraGrab::RemoveHead(char**ppBuffer,DWORD*pLen)
{
	pair<char*, DWORD > frame(nullptr,0);
	while (true)
	{
		WaitForSingleObject(m_hMutex, INFINITE);
		if (m_FrameList.size()){
			frame = m_FrameList.front();
			m_FrameList.pop_front();
		}

		if (!frame.first)								//buffer 为空.
			ResetEvent(m_hEvent);						//如果没有数据了...,
		else
			m_dwBufSize -= frame.second;

		SetEvent(m_hMutex);

		if (!frame.first){
			DWORD dwResult = 0;
			for (int try_count = 0; try_count < 10; try_count++){
				dwResult = WaitForSingleObject(m_hEvent, 200);
				if (dwResult == WAIT_OBJECT_0){
					break;
				}
				if (m_IsGrabbing == FALSE){
					break;
				}
			}
			if (WAIT_TIMEOUT == dwResult){
				//有事件时,这时候链表里面就有数据了...
				*ppBuffer = NULL;
				*pLen = NULL;
				return;
			}
		}
		else
			break;
	}
	*ppBuffer = frame.first;
	*pLen = frame.second;
}

VideoInfoList& CCameraGrab::GetDeviceList()
{ 
	m_device.clear();
	GetDeviceList(string(""), 0);
	return m_device;
}

void CCameraGrab::GetDeviceList(const string& query_name, IBaseFilter**ppBaseFilter)
{
	HRESULT hr = NULL;
	ICreateDevEnum*pDevEnum = NULL;
	IEnumMoniker*pEnumMoniker = NULL;
	IMoniker*pMoniker = NULL;
	ICaptureGraphBuilder2 *pBuilder = NULL;
	BOOL	bLoop = TRUE;
	int i = 0;

	hr = CoCreateInstance(
		(REFCLSID)CLSID_CaptureGraphBuilder2,
		NULL, 
		CLSCTX_INPROC, 
		(REFIID)IID_ICaptureGraphBuilder2,
		(void **)&pBuilder);

	if (FAILED(hr)){
		goto Failed;
	}

	hr = CoCreateInstance(
		CLSID_SystemDeviceEnum, 
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, 
		(void **)&pDevEnum);
	
	if (FAILED(hr) || !pDevEnum){	
		goto Failed;
	}

	hr = pDevEnum->CreateClassEnumerator(
		CLSID_VideoInputDeviceCategory,
		&pEnumMoniker,
		0);
	
	if (FAILED(hr) || !pEnumMoniker){	
		goto Failed;
	}

	for (hr = pEnumMoniker->Next(1, &pMoniker, NULL);
		hr == S_OK&&pMoniker&&bLoop;
		hr = pEnumMoniker->Next(1, &pMoniker, NULL)){

		VARIANT varTemp;
		IPropertyBag *pProBag = NULL;
		string device_name;

		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (LPVOID*)&pProBag);
		if (hr != S_OK || pProBag == NULL){
			goto failed;
		}
		//获取设备名称....
		varTemp.vt = VT_BSTR;
		hr = pProBag->Read(L"FriendlyName", &varTemp, NULL);
		
		if (!SUCCEEDED(hr)){
			goto failed;
		}

		WCHAR * w_name = varTemp.bstrVal;
		//wide byte to g2312....
		int len = WideCharToMultiByte(CP_ACP, 0, w_name, lstrlenW(w_name), nullptr, 0, nullptr, nullptr);
		
		device_name.resize(len,0);
		
		WideCharToMultiByte(CP_ACP, 0, w_name, lstrlenW(w_name), 
			(char*)device_name.c_str(), len, nullptr, nullptr);

		SysFreeString(varTemp.bstrVal);

		if (!query_name.length()){			//获取所有设备信息..

			IBaseFilter*	pBaseFilter = NULL;
			IAMStreamConfig *pVSC = NULL;

			hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (LPVOID*)&pBaseFilter);

			if (!SUCCEEDED(hr) || pBaseFilter == NULL){
				if (pBaseFilter) pBaseFilter->Release();
				goto failed;
			}

			//这两个有啥区别呢????
			hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
				pBaseFilter, IID_IAMStreamConfig, (void **)&pVSC);

			if (hr != NOERROR)
			{
				hr = pBuilder->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
					pBaseFilter, IID_IAMStreamConfig, (void **)&pVSC);
			}

			if (SUCCEEDED(hr)){
				int iCount = 0, iSize = 0;
				hr = pVSC->GetNumberOfCapabilities(&iCount, &iSize);

				if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
				{
					for (int iFormat = 0; iFormat < iCount; iFormat++)
					{
						VIDEO_STREAM_CONFIG_CAPS scc;
						AM_MEDIA_TYPE *pmtConfig = NULL;
						hr = pVSC->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
						if (SUCCEEDED(hr))
						{
							//分辨率
							if (HEADER(pmtConfig->pbFormat)->biWidth != 0 &&
								HEADER(pmtConfig->pbFormat)->biHeight != 0)
							{
								int iWidth = HEADER(pmtConfig->pbFormat)->biWidth;
								int iHeight = HEADER(pmtConfig->pbFormat)->biHeight;
								//为什么这种方法获取的分辨率要少?
								dbg_log("Compression:%x ,BitCount:%d, Width : %d, Height: %d",
									HEADER(pmtConfig->pbFormat)->biCompression, 
									HEADER(pmtConfig->pbFormat)->biBitCount,
									iWidth, iHeight);

								if (m_device[device_name].find(pair<int,int>(iWidth,iHeight))
									== m_device[device_name].end()){
									m_device[device_name].insert(pair<int, int>(iWidth, iHeight));
								}
							}
							//Delete Media type.
							_DeleteMediaType(pmtConfig);
						}
					}
				}
				pVSC->Release();
			}

			pBaseFilter->Release(), pBaseFilter = nullptr;
		}
		else if (query_name.length() && query_name == device_name){				//查询指定的设备..
			IBaseFilter*	pBaseFilter = NULL;
			bLoop = FALSE;
			hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (LPVOID*)&pBaseFilter);
			if (SUCCEEDED(hr) && pBaseFilter){
				*ppBaseFilter = pBaseFilter;
			}
		}
		//
	failed:
		if (pMoniker)
			pMoniker->Release();
		if (pProBag)
			pProBag->Release();
	}

Failed:
	if (pEnumMoniker)
		pEnumMoniker->Release();

	if (pDevEnum)
		pDevEnum->Release();
}

int CCameraGrab::GrabberInit(const string&device_name, DWORD dwWidth, DWORD dwHeight, char*szError)
{
	HRESULT hr = NULL;
	AM_MEDIA_TYPE mt;
	AM_MEDIA_TYPE * mmt = NULL;
	IAMStreamConfig   *pConfig = NULL;
	VIDEOINFOHEADER * pvih = NULL;
	x264_param_t param = { 0 };
	DWORD WidthBytes = 0;

	GrabberTerm();

	m_dwHeight = dwHeight;
	m_dwWidth = dwWidth;
	//
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&m_pGraph);
	if (FAILED(hr)){
		wsprintfA(szError, "CoCreateInstance 1 Failed");
		goto Failed;
	}
	//
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&m_pBuild);
	if (FAILED(hr)){
		wsprintfA(szError, ("CoCreateInstance 2 Failed"));
		goto Failed;
	}
	//
	hr = m_pBuild->SetFiltergraph(m_pGraph);
	if (FAILED(hr)){
		wsprintfA(szError, ("m_pBuild->SetFiltergraph(m_pGraph) Failed"));
		goto Failed;
	}
	//
	GetDeviceList(device_name,&m_pCaptureFilter);

	if (!m_pCaptureFilter || (hr = m_pGraph->AddFilter(m_pCaptureFilter, L"Video Capture"), FAILED(hr))){
		wsprintfA(szError, ("m_pGraph->AddFilter Failed"));
		goto Failed;
	}
	//
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, (LPVOID*)&m_pSampleGrabberFilter);

	if (FAILED(hr)){
		wsprintfA(szError, ("CoCreateInstance3 Failed"));
		goto Failed;
	}
	//
	hr = m_pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&m_pSamplerGrabber);
	
	if (FAILED(hr)){
		wsprintfA(szError, ("m_pSampleGrabberFilter->QueryInterface Failed"));
		goto Failed;
	}
	//
	
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	//设置输出视频格式.,SamplerGrabber的输出格式.
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_ARGB32;
	//mt.subtype = MEDIASUBTYPE_420O;
	
	//
	m_pSamplerGrabber->SetMediaType(&mt);
	m_pSamplerGrabber->SetOneShot(FALSE);
	m_pSamplerGrabber->SetBufferSamples(FALSE);
	m_pCallback = new CMySampleGrabberCB(this);
	m_pSamplerGrabber->SetCallback(m_pCallback, 1);
	//

	hr = CoCreateInstance(
		CLSID_NullRenderer,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, 
		(LPVOID*)&m_pNullRenderer);
	
	if (FAILED(hr)){
		wsprintfA(szError, ("CoCreateInstance 4 Failed"));
		goto Failed;
	}
	//
	hr = m_pGraph->AddFilter(m_pSampleGrabberFilter, L"Sample Filter");
	if (FAILED(hr)){
		wsprintfA(szError, (" m_pGraph->AddFilter(Sample Filter) Failed"));
		goto Failed;
	}
	hr = m_pGraph->AddFilter(m_pNullRenderer, L"Null Renderer");
	if (FAILED(hr)){
		wsprintfA(szError, (" m_pGraph->AddFilter(Null Renderer) Failed"));
		goto Failed;
	}
	//
	hr = m_pBuild->FindInterface(&PIN_CATEGORY_CAPTURE,
		&MEDIATYPE_Video, m_pCaptureFilter, IID_IAMStreamConfig, (void **)&pConfig);
	if (FAILED(hr)){
		wsprintfA(szError, (" m_pBuild->FindInterface Failed"));
		goto Failed;
	}


	//获取视频尺寸,先设置完尺寸再连接起来。
	//设置捕获尺寸.
	hr = pConfig->GetFormat(&mmt);    //取得默认参数
	if (FAILED(hr)){
		wsprintfA(szError, ("pConfig->GetFormat Failed"));
		goto Failed;
	}

	//只设置size 可能会失败....
	//Set Capture Video Size;

	/*
		16位位图: 每五位保存一种颜色值,有一位保留.

	*/

	pvih = (VIDEOINFOHEADER*)mmt->pbFormat;
	pvih->bmiHeader.biHeight = dwHeight;
	pvih->bmiHeader.biWidth = dwWidth;
	//四字节的整数倍.位数就是32的整数倍
	WidthBytes = ((dwWidth * pvih->bmiHeader.biBitCount + 31) & (~31)) >> 3;
	pvih->bmiHeader.biSizeImage = WidthBytes * dwHeight;

	hr = pConfig->SetFormat(mmt);
	_DeleteMediaType(mmt);
	pConfig->Release();

	if (FAILED(hr)){
		wsprintfA(szError, ("pConfig->SetFormat(mmt) Failed"));
		goto Failed;
	}

	//
	hr = m_pBuild->RenderStream(
		&PIN_CATEGORY_CAPTURE, 
		&MEDIATYPE_Video,
		m_pCaptureFilter, 
		m_pSampleGrabberFilter,
		m_pNullRenderer);
	
	if (FAILED(hr)){
		wsprintfA(szError, ("m_pBuild->RenderStream Failed"));
		goto Failed;
	}
	//获取控制接口
	hr = m_pGraph->QueryInterface(IID_IMediaControl, (void**)&m_pMediaControl);
	if (FAILED(hr)){
		wsprintfA(szError, ("m_pGraph->QueryInterface Failed"));
		goto Failed;
	}
	/*
			x264 encoder init....
	*/
	m_pPicIn = (x264_picture_t*)calloc(1,sizeof(x264_picture_t));
	m_pPicOut = (x264_picture_t*)calloc(1,sizeof(x264_picture_t));

	x264_picture_init(m_pPicIn);
	x264_picture_alloc(m_pPicIn, X264_CSP_I420, m_dwWidth, m_dwHeight);

	x264_param_default_preset(&param, "ultrafast", "zerolatency");

	param.i_width = m_dwWidth;
	param.i_height = m_dwHeight;

	param.i_log_level = X264_LOG_NONE;
	param.i_threads = 1;
	param.i_frame_total = 0;
	param.i_keyint_max = 10;
	param.i_bframe = 0;					//不启用b帧
	param.b_open_gop = 0;

	param.i_fps_num = 30;
	param.i_csp = X264_CSP_I420;

	x264_param_apply_profile(&param, x264_profile_names[0]);

	m_pEncoder = x264_encoder_open(&param);
	if (!m_pEncoder){
		wsprintfA(szError, ("x264_encoder_open Failed"));
		goto Failed;
	}
	//
	m_hMutex = CreateEvent(NULL, FALSE, TRUE, NULL);
	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	wsprintfA(szError, ("grab init succeeded."));;
	return 0;
Failed:
	GrabberTerm();
	return -1;
}

BOOL CCameraGrab::GetFrame(BYTE**lppbuffer,UINT32 * plen)
{
	//printf("Get Frame()\n");
	char*Buffer = nullptr;
	DWORD dwLen = 0;
	x264_nal_t*pNal = NULL;
	int			iNal = 0;
	int			Size = 0;

	RemoveHead(&Buffer, &dwLen);
	if (Buffer == nullptr){
		//printf("No Buffer\n");
		return FALSE;
	}
	//to I420
	libyuv::ARGBToI420((uint8_t*)Buffer, m_dwWidth * 4, m_pPicIn->img.plane[0], m_pPicIn->img.i_stride[0],
		m_pPicIn->img.plane[1], m_pPicIn->img.i_stride[1], m_pPicIn->img.plane[2], 
		m_pPicIn->img.i_stride[2],m_dwWidth,m_dwHeight);
	
	
	Size = x264_encoder_encode(m_pEncoder, &pNal, &iNal, m_pPicIn, m_pPicOut);
	delete[] Buffer;

	if (Size < 0)
		return FALSE;

	*lppbuffer = pNal[0].p_payload;
	*plen = Size;
	return TRUE;
}

BOOL CCameraGrab::StartGrab()
{
	HRESULT hr = 0;
	if (m_pMediaControl &&
		(hr = m_pMediaControl->Run(), SUCCEEDED(hr)))
	{
		m_IsGrabbing = TRUE;
		return TRUE;
	}
	return FALSE;
}

void CCameraGrab::StopGrab()
{
	if (m_pMediaControl){
		m_pMediaControl->Stop();
		m_IsGrabbing = FALSE;
	}
}