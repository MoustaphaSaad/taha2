#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/websocket/Client.h>

int main()
{
	core::Mallocator mallocator;
	core::Log logger{&mallocator};
	auto config = core::websocket::ClientConfig
	{
		.ip = "127.0.0.1"_sv,
		.port = "8080"_sv,
	};
	auto clientResult = core::websocket::Client::connect(std::move(config), &logger, &mallocator);
	if (clientResult.isError())
	{
		logger.error("failed to connect to websocket server, {}"_sv, clientResult.error());
		return EXIT_FAILURE;
	}
	auto client = clientResult.releaseValue();
	return 0;
}