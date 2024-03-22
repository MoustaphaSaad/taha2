#include <core/Mallocator.h>
#include <core/Log.h>

int main()
{
	core::Mallocator allocator{};
	core::Log log{&allocator};

	log.info("Hello, World!\n"_sv);
	return 0;
}