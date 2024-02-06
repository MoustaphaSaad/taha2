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

class PingEvent: public core::Event2
{
public:
	core::EventThread* pingThread = nullptr;

	explicit PingEvent(core::EventThread* thread)
		: pingThread(thread)
	{}
};

class PongEvent: public core::Event2
{
public:
	core::EventThread* pongThread = nullptr;

	explicit PongEvent(core::EventThread* thread)
		: pongThread(thread)
	{}
};

class PongThread: public core::EventThread
{
	core::Allocator* m_allocator = nullptr;
	core::Log* m_log = nullptr;
public:
	explicit PongThread(core::EventThreadPool* eventThreadPool, core::Log* log, core::Allocator* allocator)
		: core::EventThread(eventThreadPool),
		  m_allocator(allocator),
		  m_log(log)
	{}

	void handle(core::Event2* event) override
	{
		if (auto pingEvent = dynamic_cast<PingEvent*>(event))
		{
			m_log->info("ping received"_sv);
			(void)eventThreadPool()->sendEvent(core::unique_from<PongEvent>(m_allocator, this), pingEvent->pingThread);
		}
		else
		{
			m_log->info("unknown event"_sv);
		}
	}
};

class PingThread: public core::EventThread
{
	core::Allocator* m_allocator = nullptr;
	core::Log* m_log = nullptr;
public:
	explicit PingThread(core::EventThreadPool* eventThreadPool, core::Log* log, core::Allocator* allocator)
		: core::EventThread(eventThreadPool),
		  m_allocator(allocator),
		  m_log(log)
	{}

	void handle(core::Event2* event) override
	{
		if (auto pongEvent = dynamic_cast<PongEvent*>(event))
		{
			m_log->info("pong received"_sv);
			sendPing(pongEvent->pongThread);
		}
		else
		{
			m_log->info("unknown event"_sv);
		}
	}

	void sendPing(core::EventThread* thread)
	{
		(void)eventThreadPool()->sendEvent(core::unique_from<PingEvent>(m_allocator, this), thread);
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

	auto pingThread = pool->startThread<PingThread>(pool.get(), &log, &allocator);
	auto pongThread = pool->startThread<PongThread>(pool.get(), &log, &allocator);

	pingThread->sendPing(pongThread);

	auto err = pool->run();
	if (err)
	{
		log.critical("event thread pool error, {}"_sv, err);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}