#include "core/Log.h"
#include "core/Mallocator.h"
#include "core/Socket.h"
#include "core/Thread.h"
#include "core/Array.h"
#include <signal.h>

core::Socket* LISTEN_SOCKET = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		LISTEN_SOCKET->shutdown(core::Socket::SHUTDOWN_RDWR);
	}
}

void handleClient(core::Socket* socket)
{
	std::byte line[1024] = {};
	while (true)
	{
		auto readSize = socket->read(line, sizeof(line));
		if (readSize == 0)
			return;

		auto writeSize = socket->write(line, readSize);
		assert(writeSize == readSize);
	}
}

int main()
{
	core::Mallocator mallocator{};
	core::Log log{&mallocator};

	auto listenSocket = core::Socket::open(&mallocator, core::Socket::FAMILY_IPV4, core::Socket::TYPE_TCP);
	if (listenSocket == nullptr)
	{
		log.critical("failed to create listening socket"_sv);
		return EXIT_FAILURE;
	}

	auto ok = listenSocket->bind("localhost"_sv, "8080"_sv);
	if (ok == false)
	{
		log.critical("failed to bind listening socket"_sv);
		return EXIT_FAILURE;
	}

	ok = listenSocket->listen();
	if (ok == false)
	{
		log.critical("failed to start listening"_sv);
		return EXIT_FAILURE;
	}

	core::Array<core::Unique<core::Socket>> sockets{&mallocator};
	core::Array<core::Thread> threads{&mallocator};

	while (auto clientSocket = listenSocket->accept())
	{
		core::Thread thread{&mallocator, [socket = clientSocket.get()]{ handleClient(socket); }};
		sockets.push(std::move(clientSocket));
		threads.push(std::move(thread));
	}

	for (auto& socket: sockets)
		socket->shutdown(core::Socket::SHUTDOWN_RD);

	for (auto& thread: threads)
		thread.join();

	return 0;
}