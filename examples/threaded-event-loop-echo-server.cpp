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

class EchoThread: public core::EventThread
{
	core::EventSocket m_socket;
public:
	EchoThread(core::EventThreadPool* eventThreadPool, core::EventSocket socket)
		: core::EventThread(eventThreadPool),
		  m_socket(std::move(socket))
	{}

	void handle(core::Event2* event) override
	{
		auto eventLoop = eventThreadPool();

		if (auto startEvent = dynamic_cast<core::StartEvent*>(event))
		{
			ZoneScopedN("EchoThread::StartEvent");
			(void)m_socket.read(sharedFromThis());
		}
		else if (auto readEvent = dynamic_cast<core::ReadEvent2*>(event))
		{
			ZoneScopedN("EchoThread::ReadEvent");
			if (readEvent->bytes().count() == 0)
			{
				stop();
				return;
			}

			(void)m_socket.write(readEvent->bytes(), sharedFromThis());
			(void)m_socket.read(sharedFromThis());
		}
	}
};


class AcceptThread: public core::EventThread
{
	core::EventSocket m_socket;
public:
	AcceptThread(core::EventThreadPool* eventThreadPool, core::EventSocket socket)
		: core::EventThread(eventThreadPool),
		  m_socket(std::move(socket))
	{}

	void handle(core::Event2* event) override
	{
		auto eventLoop = eventThreadPool();

		if (auto startEvent = dynamic_cast<core::StartEvent*>(event))
		{
			ZoneScopedN("AcceptThread::StartEvent");
			(void)m_socket.accept(sharedFromThis());
		}
		else if (auto acceptEvent = dynamic_cast<core::AcceptEvent2*>(event))
		{
			ZoneScopedN("AcceptThread::AcceptEvent");
			auto conn = acceptEvent->releaseSocket();
			auto eventSocketResult = eventLoop->registerSocket(std::move(conn));
			if (eventSocketResult.isError() == false)
			{
				auto eventSocket = eventSocketResult.releaseValue();
				eventLoop->startThread<EchoThread>(eventLoop, std::move(eventSocket));
				(void)m_socket.accept(sharedFromThis());
			}

		}
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

	auto acceptSocket = core::Socket::open(&allocator, core::Socket::FAMILY_IPV4, core::Socket::TYPE_TCP);
	acceptSocket->bind("localhost"_sv, "8080"_sv);
	acceptSocket->listen();
	auto eventSocketResult = pool->registerSocket(std::move(acceptSocket));
	if (eventSocketResult.isError())
	{
		log.critical("failed to register accept socket, {}"_sv, eventSocketResult.releaseError());
		return EXIT_FAILURE;
	}
	auto eventSocket = eventSocketResult.releaseValue();

	auto acceptThread = pool->startThread<AcceptThread>(pool.get(), std::move(eventSocket));

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