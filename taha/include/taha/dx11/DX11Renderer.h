#pragma once

#include "taha/Exports.h"
#include "taha/Renderer.h"

#include <core/Result.h>
#include <core/Unique.h>

class IDXGIFactory1;
class IDXGIAdapter1;
class ID3D11Device;
class ID3D11DeviceContext;
namespace taha
{
	class DX11Renderer: public Renderer
	{
		template<typename T, typename... TArgs>
		friend inline core::Unique<T> core::unique_from(core::Allocator* allocator, TArgs&&... args);

		core::Allocator* m_allocator = nullptr;
		IDXGIFactory1* m_factory = nullptr;
		IDXGIAdapter1* m_adapter = nullptr;
		ID3D11Device* m_device = nullptr;
		ID3D11DeviceContext* m_deviceContext = nullptr;

		DX11Renderer(IDXGIFactory1* factory, IDXGIAdapter1* adapter, ID3D11Device* device, ID3D11DeviceContext* deviceContext, core::Allocator* allocator)
			: m_allocator(allocator),
			  m_factory(factory),
			  m_adapter(adapter),
			  m_device(device),
			  m_deviceContext(deviceContext)
		{}

	public:
		TAHA_EXPORT static core::Result<core::Unique<DX11Renderer>> create(core::Allocator* allocator);

		TAHA_EXPORT core::Unique<Frame> createFrameForWindow(NativeWindowDesc desc) override;
	};
}