#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>
#include <core/EventLoop.h>

#include <tracy/Tracy.hpp>

#include <signal.h>

core::ThreadedEventLoop* LOOP;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		LOOP->stop();
	}
}

class EchoThread: public core::EventThread
{
	core::EventSocket m_socket;
public:
	explicit EchoThread(core::EventSocket socket, core::EventLoop* eventLoop)
		: EventThread(eventLoop),
		  m_socket(std::move(socket))
	{}

	core::HumanError handle(core::Event* event) override
	{
		if (auto startEvent = dynamic_cast<core::StartEvent2*>(event))
		{
			ZoneScopedN("EchoThread::StartEvent");
			return m_socket.read(sharedFromThis());
		}
		else if (auto readEvent = dynamic_cast<core::ReadEvent*>(event))
		{
			ZoneScopedN("EchoThread::ReadEvent");
			if (readEvent->bytes().count() == 0)
			{
				return stop();
			}

			if (auto err = m_socket.write(readEvent->bytes(), nullptr))
				return err;
			return m_socket.read(sharedFromThis());
		}
		return {};
	}
};

class AcceptThread: public core::EventThread
{
	core::EventSocket m_socket;
	core::Shared<core::ThreadedEventLoop> m_threadedEventLoop;
public:
	explicit AcceptThread(core::EventSocket socket, core::EventLoop* eventLoop, core::Shared<core::ThreadedEventLoop> threadedEventLoop)
		: EventThread(eventLoop),
		  m_socket(std::move(socket)),
		  m_threadedEventLoop(threadedEventLoop)
	{}

	core::HumanError handle(core::Event* event) override
	{
		if (auto startEvent = dynamic_cast<core::StartEvent2*>(event))
		{
			ZoneScopedN("AcceptThread::StartEvent");
			return m_socket.accept(sharedFromThis());
		}
		else if (auto acceptEvent = dynamic_cast<core::AcceptEvent2*>(event))
		{
			ZoneScopedN("AcceptThread::AcceptEvent");
			auto loop = m_threadedEventLoop->next();
			auto socketResult = loop->registerSocket(acceptEvent->releaseSocket());
			if (socketResult.isError())
				return socketResult.releaseError();
			loop->startThread<EchoThread>(socketResult.releaseValue(), loop);
			return m_socket.accept(sharedFromThis());
		}
		return {};
	}
};

int main()
{
	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto eventLoopResult = core::ThreadedEventLoop::create(&log, &allocator);
	if (eventLoopResult.isError())
	{
		log.critical("failed to create event thread pool, {}"_sv, eventLoopResult.releaseError());
		return EXIT_FAILURE;
	}
	auto loop = eventLoopResult.releaseValue();
	LOOP = loop.get();

	auto socket = core::Socket::open(&allocator, core::Socket::FAMILY_IPV4, core::Socket::TYPE_TCP);
	if (socket == nullptr)
	{
		log.critical("failed to open socket"_sv);
		return EXIT_FAILURE;
	}

	auto ok = socket->bind("localhost"_sv, "8080"_sv);
	if (ok == false)
	{
		log.critical("failed to bind socket"_sv);
		return EXIT_FAILURE;
	}

	ok = socket->listen();
	if (ok == false)
	{
		log.critical("failed to listen socket"_sv);
		return EXIT_FAILURE;
	}

	auto nextLoop = loop->next();
	auto eventSocketResult = nextLoop->registerSocket(std::move(socket));
	if (eventSocketResult.isError())
	{
		log.critical("failed to register socket, {}"_sv, eventSocketResult.releaseError());
		return EXIT_FAILURE;
	}
	auto eventSocket = eventSocketResult.releaseValue();
	nextLoop->startThread<AcceptThread>(eventSocket, nextLoop, loop);

	// core::Thread thread{&allocator, []{
	// 	std::this_thread::sleep_for(std::chrono::seconds(20));
	// 	LOOP->stop();
	// }};

	auto err = loop->run();
	if (err)
	{
		log.critical("send thread pool error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	log.info("success"_sv);
	return EXIT_SUCCESS;
}