#include "core/VirtualMem.h"

#include <tracy/Tracy.hpp>

#include <sys/mman.h>

#include <cassert>

namespace core
{
	void* VirtualMem::alloc(size_t size, size_t)
	{
		auto res = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		TracyAllocS(res, size, 10);
		return res;
	}

	void VirtualMem::commit(void* ptr, size_t size)
	{
		[[maybe_unused]] auto res = mprotect(ptr, size, PROT_READ | PROT_WRITE);
		assert(res == 0);
	}

	void VirtualMem::release(void* ptr, size_t size)
	{
		[[maybe_unused]] auto res = mprotect(ptr, size, PROT_NONE);
		assert(res == 0);
	}

	void VirtualMem::free(void* ptr, size_t size)
	{
		TracyFreeS(ptr, 10);
		munmap(ptr, size);
	}
}