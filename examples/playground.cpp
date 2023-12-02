#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/Websocket.h>
#include <signal.h>

core::Unique<core::websocket::Server> server;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		server->stop();
	}
}

core::HumanError onMsg(const core::websocket::Msg& msg, core::websocket::Connection* conn, core::Log* log)
{
	switch (msg.type)
	{
	case core::websocket::Msg::TYPE_TEXT:
//		log->debug("msg: {}"_sv, core::StringView{msg.payload});
		if (auto err = conn->writeText(msg.payload)) return err;
		break;
	case core::websocket::Msg::TYPE_BINARY:
		if (auto err = conn->writeBinary(msg.payload)) return err;
		break;
	default:
		assert(false);
		break;
	}
	return {};
}

int main()
{
	signal(SIGINT, signalHandler);

	core::Mallocator mallocator;
	core::Log logger{&mallocator};

	logger.info("Hello, World!\n"_sv);

	auto serverResult = core::websocket::Server::open("172.25.48.1"_sv, "9010"_sv, &logger, &mallocator);
	if (serverResult.isError())
	{
		logger.error("opening websocket server failed, {}"_sv, serverResult.error());
		return EXIT_FAILURE;
	}
	server = serverResult.releaseValue();

	core::websocket::Handler handler;
	handler.onMsg = [&logger](const core::websocket::Msg& msg, core::websocket::Connection* conn){ return onMsg(msg, conn, &logger); };
	auto err = server->run(&handler);
	if (err) logger.error("websocket run failed, {}"_sv, err);

	return 0;
}