#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/Websocket.h>

core::HumanError onMsg(const core::websocket::Msg& msg, core::Log* log)
{
	log->info("msg: {}"_sv, core::StringView{msg.payload});
	return {};
}

int main()
{
	core::Mallocator mallocator;
	core::Log logger{&mallocator};

	logger.info("Hello, World!\n"_sv);

	auto serverResult = core::websocket::Server::open("127.0.0.1"_sv, "8080"_sv, &logger, &mallocator);
	if (serverResult.isError())
	{
		logger.error("opening websocket server failed, {}"_sv, serverResult.error());
		return EXIT_FAILURE;
	}
	auto server = serverResult.releaseValue();

	core::websocket::Handler handler;
	handler.onMsg = [&logger](const core::websocket::Msg& msg, core::websocket::Connection* conn) { return onMsg(msg, &logger); };
	auto err = server->run(&handler);
	if (err) logger.error("websocket run failed, {}"_sv, err);

	return 0;
}