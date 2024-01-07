#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/EventLoop.h>
#include <core/Socket.h>

#include <signal.h>

core::EventLoop* EVENT_LOOP = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		EVENT_LOOP->stop();
	}
}

class Acceptor: public core::Reactor
{
	core::Log* m_log = nullptr;
public:
	explicit Acceptor(core::Log* log)
		: m_log(log)
	{}

	void onAccept(core::AcceptEvent* event) override
	{
		// create printer here and do stuff with it
		auto socket = event->releaseSocket();
		// reschedule accept
		event->eventLoop()->accept(event->source(), this);
		m_log->debug("accepted socket: {}"_sv, socket->fd());
	}
};

class Printer: public core::Reactor
{
	core::Log* m_log = nullptr;
public:
	explicit Printer(core::Log* log)
		: m_log(log)
	{}

	void onRead(const core::ReadEvent* event) override
	{
		m_log->debug("read: \"{}\""_sv, core::StringView{event->buffer()});
		event->eventLoop()->read(event->source(), this);
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

	Acceptor acceptor{&log};
	Printer printer{&log};

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

	// auto eventSocket = eventLoop->createEventSource(std::move(socket));
	// auto err = eventLoop->read(eventSocket.get(), &printer);
	// if (err)
	// {
	// 	log.critical("socket read error, {}"_sv, err);
	// 	return EXIT_FAILURE;
	// }

	err = eventLoop->run();
	if (err)
	{
		log.critical("event loop error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	return 0;
}