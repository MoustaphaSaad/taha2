#include "core/Websocket.h"
#include "core/Hash.h"

#include <Windows.h>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

#include <assert.h>

namespace core
{
	class WinOSServer: public Server
	{
		class Op: public OVERLAPPED
		{
		public:
			enum KIND
			{
				KIND_NONE,
				KIND_ACCEPT,
			};

			Op(KIND kind)
				: m_kind(kind)
			{}

			virtual ~Op() = default;

			KIND kind() const { return m_kind; }
		private:
			KIND m_kind = KIND_NONE;
		};

		class AcceptOp: public Op
		{
			SOCKET m_acceptSocket = INVALID_SOCKET;
			std::byte m_buffer[2 * (sizeof(SOCKADDR_IN) + 16)];
		public:
			AcceptOp()
				: Op(KIND_ACCEPT)
			{
				m_acceptSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			}

			~AcceptOp() override
			{
				if (m_acceptSocket != INVALID_SOCKET)
				{
					[[maybe_unused]] auto res = ::closesocket(m_acceptSocket);
					assert(res == 0);
				}
			}

			SOCKET acceptSocket() const
			{
				return m_acceptSocket;
			}

			void* buffer()
			{
				return m_buffer;
			}
		};

		Allocator* m_allocator = nullptr;
		HANDLE m_port = INVALID_HANDLE_VALUE;
		SOCKET m_listenSocket = INVALID_SOCKET;
		Map<Op*, Unique<Op>> m_scheduledOperations;

		template<typename T>
		T* getOp()
		{
			auto op = core::unique_from<T>(m_allocator);
			op->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			auto rawOp = op.get();
			m_scheduledOperations.insert(rawOp, std::move(op));
			return rawOp;
		}

		void putOp(Op* op)
		{
			CloseHandle(op->hEvent);
			m_scheduledOperations.remove(op);
		}

		HumanError scheduleAccept()
		{
			auto op = getOp<AcceptOp>();
			// TODO: we can receive on accept operation, it's a little bit more efficient so consider doing that in the future
			DWORD bytesReceived = 0;
			auto res = AcceptEx(
				m_listenSocket,
				op->acceptSocket(),
				op->buffer(),
				0,
				sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16,
				&bytesReceived,
				(OVERLAPPED*)op
			);
			if (res == TRUE)
			{
				handleAccept();
			}
			else
			{
				auto error = WSAGetLastError();
				if (error != ERROR_IO_PENDING)
				{
					return errf(m_allocator, "Failed to schedule accept operation: ErrorCode({})"_sv, error);
				}
			}
		}

		void handleAccept()
		{
			// TODO: handle accept here
		}

	public:
		WinOSServer(HANDLE port, SOCKET listenSocket, Allocator* allocator)
			: m_allocator(allocator),
			  m_port(port),
			  m_listenSocket(listenSocket),
			  m_scheduledOperations(allocator)
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

		virtual HumanError run() override
		{
			// schedule an accept operation
			if (auto err = scheduleAccept()) return err;

			while (true)
			{
				DWORD bytesReceived = 0;
				ULONG_PTR completionKey = 0;
				OVERLAPPED* overlapped = nullptr;
				auto res = GetQueuedCompletionStatus(m_port, &bytesReceived, &completionKey, &overlapped, INFINITE);
				if (res == FALSE)
				{
					auto error = GetLastError();
					return errf(m_allocator, "Failed to get queued completion status: ErrorCode({})"_sv, error);
				}

				auto op = (Op*)overlapped;
				switch (op->kind())
				{
				case Op::KIND_ACCEPT:
					handleAccept();
					break;
				default:
					assert(false);
					break;
				}
			}

			return {};
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

		return core::unique_from<WinOSServer>(allocator, port, listenSocket, allocator);
	}
}