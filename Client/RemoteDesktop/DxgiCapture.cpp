#include "DxgiCapture.h"
#include "dbg.h"
#pragma comment(lib,"d3d11.lib")


CDxgiCapture::CDxgiCapture()
{
	m_pDevice = NULL;
	m_pDeviceContext = NULL;
	m_pDuplication = NULL;
	m_Texture = NULL;
	m_currentOutputIdx = -1;
	memset(&m_Mapped_resource, 0, sizeof(m_Mapped_resource));
}

CDxgiCapture::~CDxgiCapture()
{
	Cleanup();
}

int CDxgiCapture::InitD3D11Device()
{

	
	D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT numFeatureLevels = sizeof(featureLevels) / sizeof(featureLevels[0]);
	D3D_FEATURE_LEVEL featureLevel;
	int error = -1;

	for (UINT i = 0; i < numDriverTypes; ++i)
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			driverTypes[i],
			nullptr,
			0,
			featureLevels,
			numFeatureLevels,
			D3D11_SDK_VERSION,
			&m_pDevice,
			&featureLevel,
			&m_pDeviceContext);

		if (SUCCEEDED(hr)) {
			break;
		}
	}

	if (!m_pDevice || !m_pDeviceContext) {
		goto failed;
	}

	return 0;

failed:
	if (m_pDevice){
		m_pDevice->Release();
		m_pDevice = NULL;
	}

	if (m_pDeviceContext){
		m_pDeviceContext->Release();
		m_pDeviceContext = NULL;
	}
	return error;

}

int CDxgiCapture::InitDuplication(int OutputIdx)
{
	HRESULT          hr = S_OK;
	IDXGIDevice*     dxgiDevice = nullptr;
	IDXGIAdapter*    dxgiAdapter = nullptr;
	IDXGIOutput*     dxgiOutput = nullptr;
	IDXGIOutput1*    dxgiOutput1 = nullptr;
	DXGI_OUTPUT_DESC desc;

	hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));

	if (FAILED(hr) || !dxgiDevice) {
		return -1;
	}
	
	hr = dxgiDevice->GetAdapter(&dxgiAdapter);
	dxgiDevice->Release();

	if (FAILED(hr) || !dxgiAdapter) {
		return -2;
	}

	hr = dxgiAdapter->EnumOutputs(OutputIdx, &dxgiOutput);
	dxgiAdapter->Release();

	if (FAILED(hr) || !dxgiOutput){
		return -3;
	}

	dxgiOutput->GetDesc(&desc);
	m_MonitorRect = desc.DesktopCoordinates;

	hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&dxgiOutput1));
	dxgiOutput->Release();

	if (FAILED(hr) || !dxgiOutput1){
		return -4;
	}

	hr = dxgiOutput1->DuplicateOutput(m_pDevice, &m_pDuplication);
	dxgiOutput1->Release();

	if (FAILED(hr) || !m_pDuplication){
		return -5;
	}

	m_currentOutputIdx = OutputIdx;
	return 0;
}

int CDxgiCapture::GetAllMonitor(RECT * lpMonitors)
{
	HRESULT hr = S_OK;

	IDXGIDevice* dxgiDevice = nullptr;
	IDXGIAdapter* dxgiAdapter = nullptr;
	IDXGIOutput* dxgiOutput = nullptr;
	DXGI_OUTPUT_DESC desc; 
	int i = 0;

	hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
	if (FAILED(hr) || !dxgiDevice){
		return -1;
	}
	
	//获取adapter
	hr = dxgiDevice->GetAdapter(&dxgiAdapter);
	dxgiDevice->Release();

	if (FAILED(hr) || !dxgiAdapter){
		return -2;
	}

	while (1){
		hr = dxgiAdapter->EnumOutputs(i, &dxgiOutput);
		if (hr == DXGI_ERROR_NOT_FOUND){
			break;
		}
		else if (dxgiOutput){
			dxgiOutput->GetDesc(&desc);
			dxgiOutput->Release();
			dxgiOutput = NULL;

			lpMonitors[i].left   = 0;
			lpMonitors[i].top    = 0;
			lpMonitors[i].right  = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
			lpMonitors[i].bottom = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
			++i;
		}
	}
	dxgiAdapter->Release();
	
	return i;
}

int CDxgiCapture::GetDesktopFrame(uint8_t ** lpRgb, uint32_t *lpStride, uint32_t* lpSize)
{
	HRESULT hr = S_OK;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	IDXGIResource* resource = nullptr;
	ID3D11Texture2D* acquireFrame = nullptr;
	
	*lpRgb    = NULL;
	*lpStride = NULL;
	*lpSize   = NULL;

acquire_frame:
	hr = m_pDuplication->AcquireNextFrame(0, &frameInfo, &resource);

	if (hr == DXGI_ERROR_ACCESS_LOST){
		/*
			如果桌面重复接口无效，DXGI_ERROR_ACCESS_LOST。 
			当桌面上显示不同类型的图像时，桌面重复界面通常会失效。 这种情况的示例包括：
			桌面交换机 , 模式更改 , 从 DWM 打开、DWM 关闭或其他全屏应用程序切换
			在这种情况下，应用程序必须释放 IDXGIOutputDuplication 接口，并为新内容
			创建新的 IDXGIOutputDuplication 。
		*/
		if (m_pDuplication){
			m_pDuplication->Release();
			m_pDuplication = NULL;
		}

		if (m_Texture){

			if (m_Mapped_resource.pData){
				m_pDeviceContext->Unmap(m_Texture, 0);
				memset(&m_Mapped_resource, 0, sizeof(m_Mapped_resource));
			}

			m_Texture->Release();
			m_Texture = NULL;
		}

		if (InitDuplication(m_currentOutputIdx)){
			return -1;
		}
			
		if (InitTexture()){
			return -2;
		}

		goto acquire_frame;

	}
	else if (hr == DXGI_ERROR_WAIT_TIMEOUT){
		goto done;
	}
	else if (FAILED(hr) || !resource){
		return -3;
	}

	hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&acquireFrame));
	resource->Release();

	if (FAILED(hr) || !acquireFrame){
		return -4;
	}

	//Copy resource to texture.
	m_pDeviceContext->CopyResource(m_Texture, acquireFrame);
	acquireFrame->Release();

	hr = m_pDuplication->ReleaseFrame();

	if (FAILED(hr)){
		return -5;
	}

done:

	*lpRgb    = (uint8_t*)m_Mapped_resource.pData;
	*lpStride = m_Mapped_resource.RowPitch;
	*lpSize   = m_Mapped_resource.DepthPitch;

	return 0;
}


int  CDxgiCapture::InitTexture(){

	D3D11_TEXTURE2D_DESC desc = { 0 };
	HRESULT hr = S_OK;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	IDXGIResource* resource = nullptr;
	ID3D11Texture2D* acquireFrame = nullptr;

	hr = m_pDuplication->AcquireNextFrame(0, &frameInfo, &resource);

	if (FAILED(hr) || !resource){
		return -1;
	}

	hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&acquireFrame));
	resource->Release();

	if (FAILED(hr) || !acquireFrame){
		return -2;
	}

	acquireFrame->GetDesc(&desc);
	acquireFrame->Release();
	m_pDuplication->ReleaseFrame();

	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;

	hr = m_pDevice->CreateTexture2D(&desc, NULL, &m_Texture);
	if (FAILED(hr) || !m_Texture){
		return -1;
	}

	hr =m_pDeviceContext->Map(m_Texture, 0, D3D11_MAP_READ, 0, &m_Mapped_resource);
	if (FAILED(hr)){
		return -2;
	}
	
	return 0;
}


void CDxgiCapture::Cleanup(){
	if (m_pDuplication){
		m_pDuplication->Release();
		m_pDuplication = NULL;
	}

	if (m_Texture){

		if (m_Mapped_resource.pData){
			m_pDeviceContext->Unmap(m_Texture, 0);
			memset(&m_Mapped_resource, 0, sizeof(m_Mapped_resource));
		}
		
		m_Texture->Release();
		m_Texture = NULL;
	}

	if (m_pDeviceContext){
		m_pDeviceContext->Release();
		m_pDeviceContext = NULL;
	}

	if (m_pDevice){
		m_pDevice->Release();
		m_pDevice = NULL;
	}
}