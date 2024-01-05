#include "core/websocket/Client.h"

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace core::websocket
{
	class WinOSClient: public Client2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		SOCKET m_socket = INVALID_SOCKET;
		ClientConfig m_config;
		Url m_url;
	public:
		WinOSClient(HANDLE completionPort, SOCKET socket, ClientConfig&& config, Url&& url, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_completionPort(completionPort),
			  m_socket(socket),
			  m_config(std::move(config)),
			  m_url(std::move(url))
		{

		}

		~WinOSClient() override
		{
			if (m_completionPort != INVALID_HANDLE_VALUE)
			{
				[[maybe_unused]] auto res = CloseHandle(m_completionPort);
				assert(SUCCEEDED(res));
			}

			if (m_socket != INVALID_SOCKET)
			{
				[[maybe_unused]] auto res = ::closesocket(m_socket);
				assert(res == 0);
			}
		}

		HumanError run() override
		{
			bool running = true;
			while (running)
			{
				// TODO: implement the client main loop here
			}
			return {};
		}
	};

	Result<Unique<Client2>> Client2::connect(ClientConfig &&config, Log *log, Allocator *allocator)
	{
		auto parsedUrlResult = core::Url::parse(config.url, allocator);
		if (parsedUrlResult.isError())
			return parsedUrlResult.releaseError();
		auto url = parsedUrlResult.releaseValue();

		auto completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
		if (completionPort == NULL)
			return errf(allocator, "failed to create completion port"_sv);

		auto socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (socket == INVALID_SOCKET)
			return errf(allocator, "failed to create socket"_sv);

		PADDRINFOA addressInfo = nullptr;
		String ipString{url.host(), allocator};
		String portString{url.port(), allocator};
		auto res = ::getaddrinfo(ipString.data(), portString.data(), nullptr, &addressInfo);
		if (res != 0)
			return errf(allocator, "failed to get address info, ErrorCode({})"_sv, res);

		bool connected = false;
		for (auto it = addressInfo; it; it = it->ai_next)
		{
			res = WSAConnect(socket, it->ai_addr, (int)it->ai_addrlen, NULL, NULL, NULL, NULL);
			if (res != SOCKET_ERROR)
			{
				connected = true;
				break;
			}
		}
		freeaddrinfo(addressInfo);
		if (connected == false)
			return errf(allocator, "failed to connect to {}"_sv, config.url);

		auto newPort = CreateIoCompletionPort((HANDLE)socket, completionPort, NULL, 0);
		assert(newPort == completionPort);

		return unique_from<WinOSClient>(allocator, completionPort, socket, std::move(config), std::move(url), log, allocator);
	}
}