#pragma once

#include "taha/Exports.h"
#include "taha/Renderer.h"
#include "taha/dx11/DXPtr.h"

#include <core/Result.h>
#include <core/Unique.h>

struct IDXGIFactory1;
struct IDXGIAdapter1;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace taha
{
	class DX11Renderer: public Renderer
	{
		template<typename T, typename... TArgs>
		friend inline core::Unique<T> core::unique_from(core::Allocator* allocator, TArgs&&... args);

		core::Allocator* m_allocator = nullptr;
		DXPtr<IDXGIFactory1> m_factory;
		DXPtr<IDXGIAdapter1> m_adapter;
		DXPtr<ID3D11Device> m_device;
		DXPtr<ID3D11DeviceContext> m_deviceContext;

		DX11Renderer(DXPtr<IDXGIFactory1> factory, DXPtr<IDXGIAdapter1> adapter, DXPtr<ID3D11Device> device, DXPtr<ID3D11DeviceContext> deviceContext, core::Allocator* allocator)
			: m_allocator(allocator),
			  m_factory(std::move(factory)),
			  m_adapter(std::move(adapter)),
			  m_device(std::move(device)),
			  m_deviceContext(std::move(deviceContext))
		{}

	public:
		TAHA_EXPORT static core::Result<core::Unique<DX11Renderer>> create(core::Allocator* allocator);

		TAHA_EXPORT core::Unique<Frame> createFrameForWindow(NativeWindowDesc desc) override;
		TAHA_EXPORT void submitCommandsAndExecute(Frame* frame, const core::Array<core::Unique<Command>>& commands) override;
	};
}