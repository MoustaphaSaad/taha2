#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/Websocket.h>

int main()
{
	core::Mallocator mallocator;
	core::Log logger{&mallocator};

	logger.info("Hello, World!\n"_sv);

	auto server = core::Server::open(&mallocator);
	auto err = server->run();
	if (err) logger.error("websocket run failed, {}"_sv, err);

	return 0;
}