#include "core/Websocket.h"

#include <Windows.h>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <assert.h>

namespace core
{
	class WinOSServer: public Server
	{
		HANDLE m_port = INVALID_HANDLE_VALUE;
		SOCKET m_listenSocket = INVALID_SOCKET;
	public:
		WinOSServer(HANDLE port, SOCKET listenSocket)
			: m_port(port),
			  m_listenSocket(listenSocket)
		{}

		~WinOSServer() override
		{
			if (m_port != INVALID_HANDLE_VALUE)
			{
				[[maybe_unused]] auto res = CloseHandle(m_port);
				assert(SUCCEEDED(res));
			}

			if (m_listenSocket != INVALID_SOCKET)
			{
				[[maybe_unused]] auto res = ::closesocket(m_listenSocket);
				assert(res == 0);
			}
		}

		virtual void run() override
		{
			// TODO: Implement this function
		}

		virtual void stop() override
		{
			// TODO: Implement this function
		}
	};

	Unique<Server> Server::open(Allocator* allocator)
	{
		auto port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
		if (port == NULL) return nullptr;

		auto listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (listenSocket == INVALID_SOCKET) return nullptr;

		return core::unique_from<WinOSServer>(allocator, port, listenSocket);
	}
}