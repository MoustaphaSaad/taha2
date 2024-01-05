#include "core/websocket/Client.h"
#include "core/Queue.h"

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace core::websocket
{

	class WinOSClient: public Client2
	{
		struct OutboundBytes
		{
			Buffer data;
			bool isScheduled = false;
		};

		struct Op: OVERLAPPED
		{
			enum KIND
			{
				KIND_NONE,
				KIND_WRITE,
			};

			explicit Op(KIND kind_)
				: kind(kind_)
			{
				::memset((OVERLAPPED*)this, 0, sizeof(OVERLAPPED));
				hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			}

			virtual ~Op()
			{
				[[maybe_unused]] auto res = CloseHandle(hEvent);
				assert(res != 0);
			}

			KIND kind = KIND_NONE;
		};

		struct WriteOp: public Op
		{
			DWORD bytesSent = 0;
			Span<const std::byte> buffer;
			WSABUF wsaBuf{};
			DWORD flags = 0;

			explicit WriteOp(Span<const std::byte> buffer_)
				: Op(KIND_WRITE),
				  buffer(buffer_)
			{
				wsaBuf.buf = (CHAR*)buffer.data();
				wsaBuf.len = (ULONG)buffer.count();
			}
		};

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		SOCKET m_socket = INVALID_SOCKET;
		ClientConfig m_config;
		Url m_url;
		Map<Op*, Unique<Op>> m_scheduledOperations;
		Queue<OutboundBytes> m_outboundQueue;

		void pushPendingOp(Unique<Op> op)
		{
			m_scheduledOperations.insert(op.get(), std::move(op));
		}

		HumanError scheduleWriteOp(Unique<WriteOp> op)
		{
			auto res = WSASend(
				m_socket,
				&op->wsaBuf,
				1,
				&op->bytesSent,
				op->flags,
				(OVERLAPPED*)op.get(),
				NULL
			);

			if (res != SOCKET_ERROR || WSAGetLastError() == ERROR_IO_PENDING)
			{
				pushPendingOp(std::move(op));
			}
			else
			{
				auto error = WSAGetLastError();
				return errf(m_allocator, "Failed to schedule write operation: ErrorCode({})"_sv, error);
			}

			return {};
		}

		HumanError processWriteQueue()
		{
			if (m_outboundQueue.count() == 0)
				return {};

			auto& outboundBytes = m_outboundQueue.front();

			if (outboundBytes.isScheduled)
				return {};

			outboundBytes.isScheduled = true;
			auto op = unique_from<WriteOp>(m_allocator, Span<const std::byte>{outboundBytes.data});
			return scheduleWriteOp(std::move(op));
		}

		HumanError scheduleWrite(Buffer&& buffer)
		{
			m_outboundQueue.push_back(OutboundBytes{.data = std::move(buffer)});
			return processWriteQueue();
		}
	public:
		WinOSClient(HANDLE completionPort, SOCKET socket, ClientConfig&& config, Url&& url, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_completionPort(completionPort),
			  m_socket(socket),
			  m_config(std::move(config)),
			  m_url(std::move(url)),
			  m_scheduledOperations(allocator),
			  m_outboundQueue(allocator)
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