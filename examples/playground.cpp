#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>
#include <core/EventLoop2.h>

#include <signal.h>

core::EventLoop2* LOOP = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		LOOP->stop();
	}
}

int main()
{
	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto eventLoopResult = core::EventLoop2::create(&log, &allocator);
	if (eventLoopResult.isError())
	{
		log.critical("{}"_sv, eventLoopResult.releaseError());
		return EXIT_FAILURE;
	}
	auto eventLoop = eventLoopResult.releaseValue();
	LOOP = eventLoop.get();

	auto err = eventLoop->run();
	if (err)
	{
		log.critical("{}"_sv, err);
		return EXIT_FAILURE;
	}

	log.info("success"_sv);
	return EXIT_SUCCESS;
}