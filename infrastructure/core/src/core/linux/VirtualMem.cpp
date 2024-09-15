#include "core/VirtualMem.h"
#include "core/Assert.h"

#include <tracy/Tracy.hpp>

#include <sys/mman.h>

namespace core
{
	Span<std::byte> VirtualMem::alloc(size_t size, size_t)
	{
		auto res = (std::byte*)mmap(nullptr, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		TracyAllocS(res, size, 10);
		return Span<std::byte>{res, size};
	}

	void VirtualMem::commit(Span<std::byte> bytes)
	{
		[[maybe_unused]] auto res = mprotect(bytes.data(), bytes.sizeInBytes(), PROT_READ | PROT_WRITE);
		validate(res == 0);
	}

	void VirtualMem::release(Span<std::byte> bytes)
	{
		[[maybe_unused]] auto res = mprotect(bytes.data(), bytes.sizeInBytes(), PROT_NONE);
		validate(res == 0);
	}

	void VirtualMem::free(Span<std::byte> bytes)
	{
		TracyFreeS(bytes.data(), 10);
		munmap(bytes.data(), bytes.sizeInBytes());
	}
}