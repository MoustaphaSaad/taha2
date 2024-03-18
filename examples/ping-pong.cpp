#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>
#include <core/EventLoop.h>

#include <tracy/Tracy.hpp>

#include <signal.h>

core::EventLoop2* LOOP;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		LOOP->stop();
	}
}

class PingEvent: public core::Event2
{
public:
	core::Shared<core::EventThread2> pingThread = nullptr;

	explicit PingEvent(const core::Shared<core::EventThread2>& thread)
		: pingThread(thread)
	{}
};

class PongEvent: public core::Event2
{
public:
	core::Shared<core::EventThread2> pongThread = nullptr;

	explicit PongEvent(const core::Shared<core::EventThread2>& thread)
		: pongThread(thread)
	{}
};

class PongThread: public core::EventThread2
{
	core::Allocator* m_allocator = nullptr;
	core::Log* m_log = nullptr;
public:
	explicit PongThread(core::EventLoop2* eventThreadPool, core::Log* log, core::Allocator* allocator)
		: core::EventThread2(eventThreadPool),
		  m_allocator(allocator),
		  m_log(log)
	{}

	core::HumanError handle(core::Event2* event) override
	{
		if (auto startEvent = dynamic_cast<core::StartEvent2*>(event))
		{
			// do nothing
			return {};
		}
		else if (auto pingEvent = dynamic_cast<PingEvent*>(event))
		{
			ZoneScopedN("PongThread::PingEvent");
			m_log->info("ping received"_sv);
			// std::this_thread::sleep_for(std::chrono::milliseconds(500));
			return pingEvent->pingThread->send(core::unique_from<PongEvent>(m_allocator, sharedFromThis()));
		}
		else
		{
			return core::errf(m_allocator, "unknown event"_sv);
		}
	}
};

class PingThread: public core::EventThread2
{
	core::Allocator* m_allocator = nullptr;
	core::Log* m_log = nullptr;
public:
	explicit PingThread(core::EventLoop2* eventThreadPool, core::Log* log, core::Allocator* allocator)
		: core::EventThread2(eventThreadPool),
		  m_allocator(allocator),
		  m_log(log)
	{}

	core::HumanError handle(core::Event2* event) override
	{
		if (auto startEvent = dynamic_cast<core::StartEvent2*>(event))
		{
			// do nothing
			return {};
		}
		else if (auto pongEvent = dynamic_cast<PongEvent*>(event))
		{
			ZoneScopedN("PingThread::PongEvent");
			m_log->info("pong received"_sv);
			// std::this_thread::sleep_for(std::chrono::milliseconds(500));
			return sendPing(pongEvent->pongThread);
		}
		else
		{
			return core::errf(m_allocator, "unknown event"_sv);
		}
	}

	core::HumanError sendPing(const core::Shared<core::EventThread2>& thread)
	{
		return thread->send(core::unique_from<PingEvent>(m_allocator, sharedFromThis()));
	}
};

int main()
{
	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto eventLoopResult = core::EventLoop2::create(&log, &allocator);
	if (eventLoopResult.isError())
	{
		log.critical("failed to create event thread pool, {}"_sv, eventLoopResult.releaseError());
		return EXIT_FAILURE;
	}
	auto loop = eventLoopResult.releaseValue();
	LOOP = loop.get();

	auto pingThread = loop->startThread<PingThread>(loop.get(), &log, &allocator);
	auto pongThread = loop->startThread<PongThread>(loop.get(), &log, &allocator);

	auto err = pingThread->sendPing(pongThread);
	if (err)
	{
		log.critical("{}"_sv, err);
		return EXIT_FAILURE;
	}

	err = loop->run();
	if (err)
	{
		log.critical("send thread pool error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	log.info("success"_sv);
	return EXIT_SUCCESS;
}