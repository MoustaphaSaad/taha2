#include "taha/dx11/DX11Renderer.h"
#include "taha/dx11/DX11Frame.h"

#include <core/Array.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>

namespace taha
{
	core::Result<core::Unique<DX11Renderer>> DX11Renderer::create(core::Allocator *allocator)
	{
		IDXGIFactory1* factory = nullptr;
		auto res = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
		if (FAILED(res))
			return core::errf(allocator, "CreateDXGIFactory1 failed, ErrorCode({})"_sv, res);

		IDXGIAdapter1* selectedAdapter1 = nullptr;
		DXGI_ADAPTER_DESC1 selectedAdapterDesc1{};
		IDXGIAdapter1* adapter1 = nullptr;
		for (UINT i = 0; factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 adapterDesc1{};
			adapter1->GetDesc1(&adapterDesc1);

			if (adapterDesc1.DedicatedVideoMemory > selectedAdapterDesc1.DedicatedVideoMemory)
			{
				if (selectedAdapter1) selectedAdapter1->Release();
				selectedAdapter1 = adapter1;
				selectedAdapterDesc1 = adapterDesc1;
			}
		}

		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		D3D_FEATURE_LEVEL features[] {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};

		UINT flags = 0;
		if (true)
			flags = D3D11_CREATE_DEVICE_DEBUG;

		res = D3D11CreateDevice(
			selectedAdapter1,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			flags,
			features,
			sizeof(features) / sizeof(*features),
			D3D11_SDK_VERSION,
			&device,
			nullptr,
			&context
		);
		if (FAILED(res))
			return core::errf(allocator, "D3D11CreateDevice failed, ErrorCode({})"_sv, res);

		return core::unique_from<DX11Renderer>(allocator, factory, selectedAdapter1, device, context, allocator);
	}

	core::Unique<Frame> DX11Renderer::createFrameForWindow(NativeWindowDesc desc)
	{
		DXGI_SWAP_CHAIN_DESC swapchainDesc{};
		swapchainDesc.BufferCount = 1;
		swapchainDesc.BufferDesc.Width = desc.width;
		swapchainDesc.BufferDesc.Height = desc.height;
		swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.OutputWindow = desc.windowHandle;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.Windowed = true;
		swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		if (desc.enableVSync)
		{
			IDXGIOutput* output = nullptr;
			auto res = m_adapter->EnumOutputs(0, &output);
			if (FAILED(res))
				return nullptr;

			UINT modesCount = 0;
			res = output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modesCount, nullptr);
			if (FAILED(res))
				return nullptr;

			core::Array<DXGI_MODE_DESC> modes{m_allocator};
			modes.resize_fill(modesCount, DXGI_MODE_DESC{});

			res = output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modesCount, modes.begin());
			if (FAILED(res))
				return nullptr;
			output->Release();

			for (const auto& mode: modes)
			{
				if (mode.Width == desc.width && mode.Height == desc.height)
					swapchainDesc.BufferDesc.RefreshRate = mode.RefreshRate;
			}
		}
		else
		{
			swapchainDesc.BufferDesc.RefreshRate.Numerator = 0;
			swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		}

		IDXGISwapChain* swapchain = nullptr;
		auto res = m_factory->CreateSwapChain(m_device, &swapchainDesc, &swapchain);
		if (FAILED(res))
			return nullptr;

		ID3D11Texture2D* colorTexture = nullptr;
		res = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&colorTexture);
		if (FAILED(res))
			return nullptr;

		ID3D11RenderTargetView* renderTargetView = nullptr;
		res = m_device->CreateRenderTargetView(colorTexture, nullptr, &renderTargetView);
		if (FAILED(res))
			return nullptr;

		ID3D11Texture2D* depthTexture = nullptr;
		D3D11_TEXTURE2D_DESC depthDesc{};
		depthDesc.Width = swapchainDesc.BufferDesc.Width;
		depthDesc.Height = swapchainDesc.BufferDesc.Height;
		depthDesc.MipLevels = 1;
		depthDesc.ArraySize = 1;
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		res = m_device->CreateTexture2D(&depthDesc, nullptr, &depthTexture);
		if (FAILED(res))
			return nullptr;

		ID3D11DepthStencilView* depthStencilView = nullptr;
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		res = m_device->CreateDepthStencilView(depthTexture, &depthStencilViewDesc, &depthStencilView);
		if (FAILED(res))
			return nullptr;

		return core::unique_from<DX11Frame>(m_allocator, swapchain, colorTexture, depthTexture, renderTargetView, depthStencilView);
	}
}