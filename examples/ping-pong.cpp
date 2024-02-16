#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>
#include <core/EventThread.h>
#include <core/ThreadPool.h>

#include <tracy/Tracy.hpp>

#include <signal.h>

core::EventThreadPool* POOL;

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
	core::Shared<core::EventThread> pingThread = nullptr;

	explicit PingEvent(const core::Shared<core::EventThread>& thread)
		: pingThread(thread)
	{}
};

class PongEvent: public core::Event2
{
public:
	core::Shared<core::EventThread> pongThread = nullptr;

	explicit PongEvent(const core::Shared<core::EventThread>& thread)
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
			ZoneScopedN("PongThread::PingEvent");
			m_log->info("ping received"_sv);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			auto err = pingEvent->pingThread->sendEvent(core::unique_from<PongEvent>(m_allocator, sharedFromThis()));
			if (err)
				m_log->critical("{}"_sv, err);
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
			ZoneScopedN("PingThread::PongEvent");
			m_log->info("pong received"_sv);
			sendPing(pongEvent->pongThread);
		}
		else
		{
			m_log->info("unknown event"_sv);
		}
	}

	void sendPing(const core::Shared<core::EventThread>& thread)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		auto err = thread->sendEvent(core::unique_from<PingEvent>(m_allocator, sharedFromThis()));
		if (err)
			m_log->critical("{}"_sv, err);
	}
};

int main()
{
	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};
	core::ThreadPool threadPool{&allocator};

	auto poolRes = core::EventThreadPool::create(&threadPool, &log, &allocator);
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
		log.critical("send thread pool error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	threadPool.flush();
	log.info("success"_sv);
	return EXIT_SUCCESS;
}