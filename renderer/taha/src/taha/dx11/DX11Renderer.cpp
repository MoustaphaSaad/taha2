#include "taha/dx11/DX11Renderer.h"
#include "taha/dx11/DX11Frame.h"
#include "taha/dx11/DXPtr.h"

#include <core/Array.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>

namespace taha
{
	core::Result<core::Unique<DX11Renderer>> DX11Renderer::create(core::Allocator* allocator)
	{
		DXPtr<IDXGIFactory1> factory;
		auto res = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)factory.getPtrAddress());
		if (FAILED(res))
		{
			return core::errf(allocator, "CreateDXGIFactory1 failed, ErrorCode({})"_sv, res);
		}

		DXPtr<IDXGIAdapter1> selectedAdapter1;
		DXGI_ADAPTER_DESC1 selectedAdapterDesc1{};
		DXPtr<IDXGIAdapter1> adapter1;
		for (UINT i = 0; factory->EnumAdapters1(i, adapter1.getPtrAddress()) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 adapterDesc1{};
			adapter1->GetDesc1(&adapterDesc1);

			if (adapterDesc1.DedicatedVideoMemory > selectedAdapterDesc1.DedicatedVideoMemory)
			{
				selectedAdapter1 = std::move(adapter1);
				selectedAdapterDesc1 = adapterDesc1;
			}
		}

		DXPtr<ID3D11Device> device;
		DXPtr<ID3D11DeviceContext> context;
		D3D_FEATURE_LEVEL features[]{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};

		UINT flags = 0;
		if (true)
		{
			flags = D3D11_CREATE_DEVICE_DEBUG;
		}

		res = D3D11CreateDevice(
			selectedAdapter1.get(),
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			flags,
			features,
			sizeof(features) / sizeof(*features),
			D3D11_SDK_VERSION,
			device.getPtrAddress(),
			nullptr,
			context.getPtrAddress());
		if (FAILED(res))
		{
			return core::errf(allocator, "D3D11CreateDevice failed, ErrorCode({})"_sv, res);
		}

		return core::unique_from<DX11Renderer>(
			allocator,
			std::move(factory),
			std::move(selectedAdapter1),
			std::move(device),
			std::move(context),
			allocator);
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
			DXPtr<IDXGIOutput> output;
			auto res = m_adapter->EnumOutputs(0, output.getPtrAddress());
			if (FAILED(res))
			{
				return nullptr;
			}

			UINT modesCount = 0;
			res = output->GetDisplayModeList(
				DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modesCount, nullptr);
			if (FAILED(res))
			{
				return nullptr;
			}

			core::Array<DXGI_MODE_DESC> modes{m_allocator};
			modes.resize_fill(modesCount, DXGI_MODE_DESC{});

			res = output->GetDisplayModeList(
				DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modesCount, modes.begin());
			if (FAILED(res))
			{
				return nullptr;
			}

			for (const auto& mode: modes)
			{
				if (mode.Width == desc.width && mode.Height == desc.height)
				{
					swapchainDesc.BufferDesc.RefreshRate = mode.RefreshRate;
				}
			}
		}
		else
		{
			swapchainDesc.BufferDesc.RefreshRate.Numerator = 0;
			swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		}

		DXPtr<IDXGISwapChain> swapchain;
		auto res = m_factory->CreateSwapChain(m_device.get(), &swapchainDesc, swapchain.getPtrAddress());
		if (FAILED(res))
		{
			return nullptr;
		}

		DXPtr<ID3D11Texture2D> colorTexture;
		res = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)colorTexture.getPtrAddress());
		if (FAILED(res))
		{
			return nullptr;
		}

		DXPtr<ID3D11RenderTargetView> renderTargetView;
		res = m_device->CreateRenderTargetView(colorTexture.get(), nullptr, renderTargetView.getPtrAddress());
		if (FAILED(res))
		{
			return nullptr;
		}

		DXPtr<ID3D11Texture2D> depthTexture;
		D3D11_TEXTURE2D_DESC depthDesc{};
		depthDesc.Width = swapchainDesc.BufferDesc.Width;
		depthDesc.Height = swapchainDesc.BufferDesc.Height;
		depthDesc.MipLevels = 1;
		depthDesc.ArraySize = 1;
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		res = m_device->CreateTexture2D(&depthDesc, nullptr, depthTexture.getPtrAddress());
		if (FAILED(res))
		{
			return nullptr;
		}

		DXPtr<ID3D11DepthStencilView> depthStencilView;
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		res = m_device->CreateDepthStencilView(
			depthTexture.get(), &depthStencilViewDesc, depthStencilView.getPtrAddress());
		if (FAILED(res))
		{
			return nullptr;
		}

		return core::unique_from<DX11Frame>(
			m_allocator,
			this,
			std::move(swapchain),
			std::move(colorTexture),
			std::move(depthTexture),
			std::move(renderTargetView),
			std::move(depthStencilView));
	}

	void DX11Renderer::submitCommandsAndExecute(Frame* frame, const core::Array<core::Unique<Command>>& commands)
	{
		// write rendering algorithm here
		// for now we get the clear command at the beginning and clear the frame
		core::validate(commands.count() > 0);
		core::validate(commands[0]->kind() == Command::KIND_CLEAR);
		auto clear = dynamic_cast<ClearCommand*>(commands[0].get());

		auto dx11Frame = dynamic_cast<DX11Frame*>(frame);
		m_deviceContext->OMSetRenderTargets(
			1, dx11Frame->m_renderTargetView.getPtrAddress(), dx11Frame->m_depthStencilView.get());

		D3D11_VIEWPORT viewport{};
		viewport.Width = 640;
		viewport.Height = 480;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		m_deviceContext->RSSetViewports(1, &viewport);

		D3D11_RECT scissor{};
		scissor.left = 0;
		scissor.right = viewport.Width;
		scissor.top = 0;
		scissor.bottom = viewport.Height;
		m_deviceContext->RSSetScissorRects(1, &scissor);

		m_deviceContext->ClearRenderTargetView(
			dx11Frame->m_renderTargetView.get(), &clear->action.color[0].value.elements.r);
		m_deviceContext->ClearDepthStencilView(
			dx11Frame->m_depthStencilView.get(),
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
			clear->action.depth.value,
			clear->action.stencil.value);

		dx11Frame->m_swapchain->Present(0, 0);
	}
}