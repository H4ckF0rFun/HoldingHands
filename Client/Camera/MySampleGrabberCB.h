#pragma once
#include "qedit.h"
class CCameraGrab;

class CMySampleGrabberCB :
	public ISampleGrabberCB
{
private:
	CCameraGrab*	m_pGrabber;
public:
	CMySampleGrabberCB(CCameraGrab*pGrabber);
	~CMySampleGrabberCB();
	STDMETHODIMP_(ULONG) AddRef() { return 2; }
	STDMETHODIMP_(ULONG) Release() { return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv) {
		if (riid == IID_ISampleGrabberCB || riid == IID_IUnknown){
			*ppv = (void *) static_cast<ISampleGrabberCB*> (this);
			return NOERROR;
		}
		return E_NOINTERFACE;
	}

	STDMETHODIMP SampleCB(double SampleTime, IMediaSample * pSample)
	{
		return 0;
	}
	HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen);
};

