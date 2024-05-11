#include <core/FastLeak.h>
#include <core/Log.h>

int main()
{
	core::FastLeak allocator{};
	core::Log log{&allocator};

	log.info("Hello, World!\n"_sv);
	return EXIT_SUCCESS;
}