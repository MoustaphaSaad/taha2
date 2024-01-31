#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>

int main()
{
	core::FastLeak allocator{};
	core::Log log{&allocator};

	int count = 0;
	core::Thread thread{&allocator, [&]() {
		count = 1;
	}};
	thread.join();
	log.info("{}"_sv, count);
	return 0;
}