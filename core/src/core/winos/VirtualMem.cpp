#include "core/VirtualMem.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace core
{
	void* VirtualMem::alloc(size_t size, size_t)
	{
		return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
	}

	void VirtualMem::commit(void* ptr, size_t size)
	{
		VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
	}

	void VirtualMem::release(void* ptr, size_t size)
	{
		VirtualFree(ptr, size, MEM_DECOMMIT);
	}

	void VirtualMem::free(void* ptr, size_t size)
	{
		VirtualFree(ptr, size, MEM_RELEASE);
	}
}