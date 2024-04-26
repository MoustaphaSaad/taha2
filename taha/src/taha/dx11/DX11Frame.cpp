#include "taha/dx11/DX11Frame.h"

#include <dxgi.h>
#include <d3d11.h>

namespace taha
{
	DX11Frame::~DX11Frame()
	{
		if (m_swapchain) m_swapchain->Release();
		if (m_colorTexture) m_colorTexture->Release();
		if (m_depthTexture) m_depthTexture->Release();
		if (m_renderTargetView) m_renderTargetView->Release();
		if (m_depthStencilView) m_depthStencilView->Release();
	}
}