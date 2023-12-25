// command to run the wstest
// docker run -it --rm
// -v "W:\Projects\taha2\test\autobahn-testsuite\config:/config"
// -v "W:\Projects\taha2\test\autobahn-testsuite\reports:/reports"
// -p 9010:9010 --name wstest crossbario/autobahn-testsuite wstest -m fuzzingclient -s /config/fuzzingclient.json

#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/websocket/Server.h>
#include <core/Url.h>
#include <signal.h>

core::websocket::Server* server = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		server->stop();
	}
}

core::HumanError onMsg(const core::websocket::Message& msg, core::websocket::Server* server, core::websocket::Connection* conn, core::Log* log)
{
	switch (msg.type)
	{
	case core::websocket::Message::TYPE_TEXT:
//		log->debug("msg: {}"_sv, core::StringView{msg.payload});
		if (auto err = server->writeText(conn, core::StringView{msg.payload})) return err;
		break;
	case core::websocket::Message::TYPE_BINARY:
		if (auto err = server->writeBinary(conn, core::Span<const std::byte>{msg.payload})) return err;
		break;
	default:
		assert(false);
		break;
	}
	return {};
}

int main(int argc, char** argv)
{
	auto url = "ws://172.25.48.1:9010"_sv;

	if (argc > 1)
		url = core::StringView{argv[1]};

	signal(SIGINT, signalHandler);

	core::Mallocator mallocator;
	core::Log logger{&mallocator};

	auto parsedUrlResult = core::Url::parse(url, &mallocator);
	if (parsedUrlResult.isError())
	{
		logger.error("parsing url failed, {}"_sv, parsedUrlResult.error());
		return EXIT_FAILURE;
	}
	auto parsedUrl = parsedUrlResult.releaseValue();

	auto serverResult = core::websocket::Server::open(parsedUrl.host(), parsedUrl.port(), &logger, &mallocator);
	if (serverResult.isError())
	{
		logger.error("opening websocket server failed, {}"_sv, serverResult.error());
		return EXIT_FAILURE;
	}
	auto wsServer = serverResult.releaseValue();
	server = wsServer.get();

	logger.info("websocket server listening on {}:{}"_sv, parsedUrl.host(), parsedUrl.port());

	core::websocket::Handler handler;
	handler.onMsg = [&logger](const core::websocket::Message& msg, core::websocket::Server* server, core::websocket::Connection* conn){ return onMsg(msg, server, conn, &logger); };
	auto err = server->run(&handler);
	if (err) logger.error("websocket run failed, {}"_sv, err);

	return 0;
}