#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>
#include <core/EventThread.h>

#include <signal.h>

core::EventThreadPool* POOL = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		POOL->stop();
	}
}

int main()
{
	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto poolRes = core::EventThreadPool::create(&log, &allocator);
	if (poolRes.isError())
	{
		log.critical("failed to create event thread pool, {}"_sv, poolRes.releaseError());
		return EXIT_FAILURE;
	}
	auto pool = poolRes.releaseValue();
	POOL = pool.get();

	auto err = pool->run();
	if (err)
	{
		log.critical("event thread pool error, {}"_sv, err);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}