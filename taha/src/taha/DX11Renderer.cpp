#include "taha/DX11Renderer.h"

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
		if (false)
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

		return core::unique_from<DX11Renderer>(allocator, factory, selectedAdapter1, device, context);
	}
}