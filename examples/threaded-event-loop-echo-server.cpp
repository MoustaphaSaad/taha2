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
	core::Unique<core::Socket> m_socket;
public:
	EchoThread(core::EventThreadPool* eventThreadPool, core::Unique<core::Socket> socket)
		: core::EventThread(eventThreadPool),
		  m_socket(std::move(socket))
	{}

	void handle(core::Event2* event) override
	{
		auto eventLoop = eventThreadPool();

		if (auto startEvent = dynamic_cast<core::StartEvent*>(event))
		{
			ZoneScopedN("EchoThread::StartEvent");
			(void)eventLoop->read(m_socket, sharedFromThis());
		}
		else if (auto readEvent = dynamic_cast<core::ReadEvent2*>(event))
		{
			ZoneScopedN("EchoThread::ReadEvent");
			(void)eventLoop->write(m_socket, sharedFromThis(), readEvent->bytes());
			(void)eventLoop->read(m_socket, sharedFromThis());
		}
	}
};


class AcceptThread: public core::EventThread
{
	core::Unique<core::Socket> m_socket;
public:
	AcceptThread(core::EventThreadPool* eventThreadPool, core::Unique<core::Socket> socket)
		: core::EventThread(eventThreadPool),
		  m_socket(std::move(socket))
	{}

	void handle(core::Event2* event) override
	{
		auto eventLoop = eventThreadPool();

		if (auto startEvent = dynamic_cast<core::StartEvent*>(event))
		{
			ZoneScopedN("AcceptThread::StartEvent");
			(void)eventLoop->accept(m_socket, sharedFromThis());
		}
		else if (auto acceptEvent = dynamic_cast<core::AcceptEvent2*>(event))
		{
			ZoneScopedN("AcceptThread::AcceptEvent");
			auto conn = acceptEvent->releaseSocket();
			(void)eventLoop->registerSocket(conn);
			eventLoop->startThread<EchoThread>(eventLoop, std::move(conn));
			(void)eventLoop->accept(m_socket, sharedFromThis());
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
	auto err = pool->registerSocket(acceptSocket);
	if (err)
	{
		log.critical("failed to register accept socket, {}"_sv, err);
		return EXIT_FAILURE;
	}

	auto acceptThread = pool->startThread<AcceptThread>(pool.get(), std::move(acceptSocket));

	err = pool->run();
	if (err)
	{
		log.critical("send thread pool error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	threadPool.flush();
	log.info("success"_sv);
	return EXIT_SUCCESS;
}