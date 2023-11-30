#include "MySampleGrabberCB.h"
#include <stdio.h>
#include"CameraGrab.h"
CMySampleGrabberCB::CMySampleGrabberCB(CCameraGrab*pGrabber)
{
	m_pGrabber = pGrabber;
}


CMySampleGrabberCB::~CMySampleGrabberCB()
{
}

HRESULT STDMETHODCALLTYPE CMySampleGrabberCB::BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen)
{
	m_pGrabber->AddTail((char*)pBuffer, BufferLen);
	return 0;
}