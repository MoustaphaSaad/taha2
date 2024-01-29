#include <doctest/doctest.h>

#include <core/ThreadPool.h>
#include <core/ExecutionQueue.h>
#include <core/Mallocator.h>
#include <core/Log.h>

TEST_CASE("core::ThreadPool basics")
{
	core::Mallocator allocator;

	std::atomic<int> count = 0;

	core::ThreadPool pool{&allocator};

	for (size_t i = 0; i < 1000; ++i)
	{
		pool.run([&]() {
			count += 1;
		});
	}

	pool.flush();

	REQUIRE(count == 1000);
}

TEST_CASE("core::ThreadPool ExecutionQueue")
{
	core::Mallocator allocator;

	constexpr int EXECUTION_QUEUES_COUNT = 10;

	std::atomic<int> count[EXECUTION_QUEUES_COUNT];
	for (int i = 0; i < EXECUTION_QUEUES_COUNT; ++i)
		count[i] = 0;

	core::Shared<core::ExecutionQueue> queues[EXECUTION_QUEUES_COUNT];
	for (int i = 0; i < EXECUTION_QUEUES_COUNT; ++i)
		queues[i] = core::ExecutionQueue::create(&allocator);

	core::ThreadPool pool{&allocator};

	for (size_t i = 0; i < 1000; ++i)
	{
		for (int qi = 0; qi < EXECUTION_QUEUES_COUNT; ++qi)
		{
			queues[qi]->push(&pool, [&, qi, i = (int)i]{
				auto val = count[qi].load();
				assert(val == i);
				count[qi] = val + 1;
			});
		}
	}

	pool.flush();

	for (int i = 0; i < EXECUTION_QUEUES_COUNT; ++i)
		assert(count[i] == 1000);
}
