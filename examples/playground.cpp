#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/websocket/Server.h>
#include <signal.h>

core::EventLoop* EVENT_LOOP = nullptr;
core::websocket::Server* SERVER = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		// stop the server
		SERVER->stop();
		// then quit the app
		EVENT_LOOP->stop();
	}
}

core::HumanError onMsg(const core::websocket::Message& msg, core::websocket::Server* server, core::websocket::Conn* conn)
{
	switch (msg.type)
	{
	case core::websocket::Message::TYPE_TEXT:
		return server->writeText(conn, core::StringView{msg.payload});
	case core::websocket::Message::TYPE_BINARY:
		return server->writeBinary(conn, core::Span<const std::byte>{msg.payload});
	default:
		assert(false);
		return {};
	}
}

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

	auto serverResult = core::websocket::Server::create(&log, &mallocator);
	if (serverResult.isError())
	{
		log.critical("failed to create websocket server, {}"_sv, eventLoopResult.error());
		return EXIT_FAILURE;
	}
	auto server = serverResult.releaseValue();

	core::websocket::ServerConfig config{};
	core::websocket::ServerHandler handler{};
	handler.onMsg = onMsg;
	auto err = server->start(config, eventLoop.get(), &handler);
	if (err)
	{
		log.critical("failed to start websocket server, {}"_sv, err);
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