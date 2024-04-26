#pragma once

#include "taha/Frame.h"
#include "taha/dx11/DXPtr.h"

struct IDXGISwapChain;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

namespace taha
{
	class DX11Frame: public Frame
	{
		DXPtr<IDXGISwapChain> m_swapchain;
		DXPtr<ID3D11Texture2D> m_colorTexture;
		DXPtr<ID3D11Texture2D> m_depthTexture;
		DXPtr<ID3D11RenderTargetView> m_renderTargetView;
		DXPtr<ID3D11DepthStencilView> m_depthStencilView;
	public:
		DX11Frame(
			DXPtr<IDXGISwapChain> swapchain,
			DXPtr<ID3D11Texture2D> colorTexture,
			DXPtr<ID3D11Texture2D> depthTexture,
			DXPtr<ID3D11RenderTargetView> renderTargetView,
			DXPtr<ID3D11DepthStencilView> depthStencilView)
			: m_swapchain(std::move(swapchain)),
			  m_colorTexture(std::move(colorTexture)),
			  m_depthTexture(std::move(depthTexture)),
			  m_renderTargetView(std::move(renderTargetView)),
			  m_depthStencilView(std::move(depthStencilView))
		{}
	};
}