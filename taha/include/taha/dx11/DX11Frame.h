#pragma once

#include "taha/Frame.h"

class IDXGISwapChain;
class ID3D11Texture2D;
class ID3D11RenderTargetView;
class ID3D11DepthStencilView;

namespace taha
{
	class DX11Frame: public Frame
	{
		IDXGISwapChain* m_swapchain = nullptr;
		ID3D11Texture2D* m_colorTexture = nullptr;
		ID3D11Texture2D* m_depthTexture = nullptr;
		ID3D11RenderTargetView* m_renderTargetView = nullptr;
		ID3D11DepthStencilView* m_depthStencilView = nullptr;
	public:
		DX11Frame(
			IDXGISwapChain* swapchain,
			ID3D11Texture2D* colorTexture,
			ID3D11Texture2D* depthTexture,
			ID3D11RenderTargetView* renderTargetView,
			ID3D11DepthStencilView* depthStencilView)
			: m_swapchain(swapchain),
			  m_colorTexture(colorTexture),
			  m_depthTexture(depthTexture),
			  m_renderTargetView(renderTargetView),
			  m_depthStencilView(depthStencilView)
		{}

		~DX11Frame() override;
	};
}