#include "core/VirtualMem.h"

#include <tracy/Tracy.hpp>

#include <Windows.h>

namespace core
{
	Span<std::byte> VirtualMem::alloc(size_t size, size_t)
	{
		auto res = (std::byte*)VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
		TracyAllocS(res, size, 10);
		return Span<std::byte>{res, size};
	}

	void VirtualMem::commit(Span<std::byte> bytes)
	{
		VirtualAlloc(bytes.data(), bytes.sizeInBytes(), MEM_COMMIT, PAGE_READWRITE);
	}

	void VirtualMem::release(Span<std::byte> bytes)
	{
		VirtualFree(bytes.data(), bytes.sizeInBytes(), MEM_DECOMMIT);
	}

	void VirtualMem::free(Span<std::byte> bytes)
	{
		TracyFreeS(bytes.data(), 10);
		VirtualFree(bytes.data(), bytes.sizeInBytes(), MEM_RELEASE);
	}
}