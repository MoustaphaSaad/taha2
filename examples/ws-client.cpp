// command to run the wstest
// docker run -it --rm
// -v "W:\Projects\taha2\test\autobahn-testsuite\config:/config"
// -v "W:\Projects\taha2\test\autobahn-testsuite\reports:/reports"
// -p 9010:9010 --name wstest crossbario/autobahn-testsuite wstest -m fuzzingserver -s /config/fuzzingserver.json

#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/websocket/Client.h>
#include <signal.h>

core::websocket::Client* client = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		client->stop();
	}
}

core::HumanError onMsg(const core::websocket::Message& msg, core::websocket::Client* client)
{
	switch (msg.type)
	{
	case core::websocket::Message::TYPE_TEXT:
		return client->writeText(core::StringView{msg.payload});
	case core::websocket::Message::TYPE_BINARY:
		return client->writeBinary(core::Span<const std::byte>{msg.payload});
	default:
		assert(false);
		return {};
	}
}

int main(int argc, char** argv)
{
	auto url = "ws://172.25.48.1:9010"_sv;

	if (argc > 1)
		url = core::StringView{argv[1]};

	core::Mallocator mallocator;
	core::Log logger{&mallocator};

	auto urlWithPath = core::strf(&mallocator, "{}/runCase?casetuple={}&agent=websocket.zig"_sv, url, "1.1.1"_sv);

	auto config = core::websocket::ClientConfig
	{
		.url = urlWithPath,
		.onMsg = onMsg,
	};
	auto clientResult = core::websocket::Client::connect(std::move(config), &logger, &mallocator);
	if (clientResult.isError())
	{
		logger.error("failed to connect to websocket server, {}"_sv, clientResult.error());
		return EXIT_FAILURE;
	}
	auto client = clientResult.releaseValue();

	auto err = client->run();
	if (err) logger.error("websocket run failed, {}"_sv, err);
	return 0;
}