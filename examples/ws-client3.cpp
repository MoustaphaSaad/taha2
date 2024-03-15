// command to run the wstest
// docker run -it --rm
// -v "W:\Projects\taha2\test\autobahn-testsuite\config:/config"
// -v "W:\Projects\taha2\test\autobahn-testsuite\reports:/reports"
// -p 9011:9011 --name wstest crossbario/autobahn-testsuite wstest -m fuzzingserver -s /config/fuzzingserver.json

#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/ThreadedEventLoop2.h>

#include <signal.h>

core::ThreadedEventLoop2* EVENT_LOOP = nullptr;
void signalHandler(int signal)
{
	if (signal == SIGINT)
		EVENT_LOOP->stop();
}

int main(int argc, char** argv)
{
	signal(SIGINT, signalHandler);

	core::FastLeak allocator;
	core::Log log{&allocator};

	auto threadedEventLoopResult = core::ThreadedEventLoop2::create(&log, &allocator);
	if (threadedEventLoopResult.isError())
	{
		log.critical("failed to create threaded event loop, {}"_sv, threadedEventLoopResult.releaseError());
		return EXIT_FAILURE;
	}
	auto threadedEventLoop = threadedEventLoopResult.releaseValue();
	EVENT_LOOP = threadedEventLoop.get();

	auto err = threadedEventLoop->run();
	if (err)
	{
		log.critical("event loop error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	log.debug("success"_sv);
	return 0;
}