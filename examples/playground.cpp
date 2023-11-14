#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/Websocket.h>

int main()
{
	core::Mallocator mallocator;
	core::Log logger{&mallocator};

	logger.info("Hello, World!\n"_sv);

	auto serverResult = core::WebSocketServer::open("127.0.0.1"_sv, "8080"_sv, &logger, &mallocator);
	if (serverResult.isError())
	{
		logger.error("opening websocket server failed, {}"_sv, serverResult.error());
		return EXIT_FAILURE;
	}
	auto server = serverResult.releaseValue();

	auto err = server->run();
	if (err) logger.error("websocket run failed, {}"_sv, err);

	return 0;
}