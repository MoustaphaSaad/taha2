#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>
#include <core/EventThread.h>
#include <core/ThreadPool.h>

#include <signal.h>

core::EventThreadPool* POOL[2];

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		POOL[0]->stop();
		POOL[1]->stop();
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
			m_log->info("ping received"_sv);
			(void)pingEvent->pingThread->sendEvent(core::unique_from<PongEvent>(m_allocator, sharedFromThis()));
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

	void sendPing(const core::Shared<core::EventThread>& thread)
	{
		(void)thread->sendEvent(core::unique_from<PingEvent>(m_allocator, sharedFromThis()));
	}
};

int main()
{
	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};
	core::ThreadPool threadPool{&allocator, 3};

	auto pool1Res = core::EventThreadPool::create(&log, &allocator);
	if (pool1Res.isError())
	{
		log.critical("failed to create event thread pool, {}"_sv, pool1Res.releaseError());
		return EXIT_FAILURE;
	}
	auto pool1 = pool1Res.releaseValue();
	POOL[0] = pool1.get();

	auto pool2Res = core::EventThreadPool::create(&log, &allocator);
	if (pool2Res.isError())
	{
		log.critical("failed to create event thread pool, {}"_sv, pool2Res.releaseError());
		return EXIT_FAILURE;
	}
	auto pool2 = pool2Res.releaseValue();
	POOL[1] = pool2.get();

	auto pingThread = pool1->startThread<PingThread>(pool1.get(), &log, &allocator);
	auto pongThread = pool2->startThread<PongThread>(pool2.get(), &log, &allocator);

	pingThread->sendPing(pongThread);

	threadPool.run([pool = pool1.get(), &log]{
		auto err = pool->run();
		if (err)
		{
			log.critical("event thread pool error, {}"_sv, err);
		}
	});

	threadPool.run([pool = pool2.get(), &log]{
		auto err = pool->run();
		if (err)
		{
			log.critical("event thread pool error, {}"_sv, err);
		}
	});

	threadPool.run([]{
		std::this_thread::sleep_for(std::chrono::seconds(5));
		POOL[0]->stop();
		POOL[1]->stop();
	});

	threadPool.flush();
	log.info("success"_sv);
	return EXIT_SUCCESS;
}