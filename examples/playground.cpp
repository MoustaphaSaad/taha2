#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/ExecutionQueue.h>
#include <core/ThreadPool.h>

int main()
{
	core::FastLeak allocator;
	core::Log log{&allocator};
	core::ThreadPool threadPool{&allocator};
	auto q = core::ExecutionQueue::create(&allocator);

	for (int i = 0; i < 100; ++i)
	{
		auto a = core::unique_from<int>(&allocator, 34);
		auto b = core::shared_from<int>(&allocator, *a);

		q->push(&threadPool, [a = std::move(a), b]{
			assert(*a == *b);
		});
	}

	threadPool.flush();

	log.info("Done"_sv);
	return 0;
}