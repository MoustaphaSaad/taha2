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
	public:
		WinOSServer(HANDLE port)
			: m_port(port)
		{}

		~WinOSServer() override
		{
			if (m_port != INVALID_HANDLE_VALUE)
			{
				[[maybe_unused]] auto res = CloseHandle(m_port);
				assert(SUCCEEDED(res));
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
		return core::unique_from<WinOSServer>(allocator, port);
	}
}