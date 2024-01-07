#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/EventLoop.h>
#include <core/Socket.h>
#include <core/Array.h>

#include <signal.h>

core::EventLoop* EVENT_LOOP = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		EVENT_LOOP->stop();
	}
}

class Echoer: public core::Reactor
{
	core::Log* m_log = nullptr;
	core::Unique<core::EventSource> m_eventSource;
public:
	explicit Echoer(core::Unique<core::EventSource> eventSource, core::Log* log)
		: m_log(log),
		  m_eventSource(std::move(eventSource))
	{
		(void)m_eventSource->eventLoop()->read(m_eventSource.get(), this);
	}

	void onRead(const core::ReadEvent* event) override
	{
		m_log->debug("read: \"{}\""_sv, core::StringView{event->buffer()});
		(void)event->eventLoop()->write(event->source(), this, event->buffer());
		(void)event->eventLoop()->read(event->source(), this);
	}

	void onWrite(const core::WriteEvent* event) override
	{
		m_log->debug("write: \"{}\""_sv, event->writtenSize());
	}
};

class Acceptor: public core::Reactor
{
	core::Allocator* m_allocator = nullptr;
	core::Log* m_log = nullptr;
	core::Array<core::Unique<Echoer>> m_clients;
public:
	explicit Acceptor(core::Log* log, core::Allocator* allocator)
		: m_allocator(allocator),
		  m_log(log),
		  m_clients(allocator)
	{}

	void onAccept(core::AcceptEvent* event) override
	{
		// create printer here and do stuff with it
		auto socket = event->releaseSocket();

		m_log->debug("accepted socket: {}"_sv, socket->fd());

		auto socketSource = event->eventLoop()->createEventSource(std::move(socket));
		m_clients.push(unique_from<Echoer>(m_allocator, std::move(socketSource), m_log));

		// reschedule accept
		(void)event->eventLoop()->accept(event->source(), this);
	}
};

int main()
{
	signal(SIGINT, signalHandler);

	core::Mallocator mallocator{};
	core::Log log{&mallocator};

	auto eventLoopResult = core::EventLoop::create(&log, &mallocator);
	if (eventLoopResult.isError())
	{
		log.critical("failed to create event loop, {}"_sv, eventLoopResult.error());
		return EXIT_FAILURE;
	}
	auto eventLoop = eventLoopResult.releaseValue();
	EVENT_LOOP = eventLoop.get();

	Acceptor acceptor{&log, &mallocator};

	auto acceptSocket = core::Socket::open(&mallocator, core::Socket::FAMILY_IPV4, core::Socket::TYPE_TCP);
	acceptSocket->bind("8080"_sv);
	acceptSocket->listen();
	auto acceptEventSocket = eventLoop->createEventSource(std::move(acceptSocket));
	auto err = eventLoop->accept(acceptEventSocket.get(), &acceptor);
	if (err)
	{
		log.critical("socket accept error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	err = eventLoop->run();
	if (err)
	{
		log.critical("event loop error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	return 0;
}