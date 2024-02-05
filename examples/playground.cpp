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

class MyThread: public core::EventThread
{
	core::Log* m_log = nullptr;
public:
	explicit MyThread(core::Log* log)
		: m_log(log)
	{}

	void handle(core::Event2* event) override
	{
		if (auto startEvent = dynamic_cast<core::StartEvent*>(event))
		{
			m_log->info("start"_sv);
		}
		else
		{
			m_log->info("unknown event"_sv);
		}
	}
};

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

	pool->startThread<MyThread>(&log);

	auto err = pool->run();
	if (err)
	{
		log.critical("event thread pool error, {}"_sv, err);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}