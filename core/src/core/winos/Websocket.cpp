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
		class Connection
		{
			SOCKET m_socket = INVALID_SOCKET;
			CHAR m_readBuffer[4 * 1024];
		public:
			Connection(SOCKET socket)
				: m_socket(socket)
			{}

			~Connection()
			{
				if (m_socket != INVALID_SOCKET)
				{
					[[maybe_unused]] auto res = ::closesocket(m_socket);
					assert(res == 0);
				}
			}

			SOCKET socket() const
			{
				return m_socket;
			}

			CHAR* readBuffer()
			{
				return m_readBuffer;
			}

			ULONG readBufferSize() const
			{
				return sizeof(m_readBuffer);
			}
		};

		class Op: public OVERLAPPED
		{
		public:
			enum KIND
			{
				KIND_NONE,
				KIND_ACCEPT,
				KIND_READ,
			};

			Op(KIND kind)
				: m_kind(kind)
			{
				memset(this, 0, sizeof(OVERLAPPED));
				hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			}

			virtual ~Op()
			{
				CloseHandle(hEvent);
			}

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

		class ReadOp: public Op
		{
			friend class WinOSServer;

			Connection* m_connection = nullptr;
			DWORD m_bytesReceived = 0;
		public:
			ReadOp(Connection* connection)
				: Op(KIND_READ),
				  m_connection(connection)
			{}
		};

		Allocator* m_allocator = nullptr;
		LPFN_ACCEPTEX m_acceptFn = nullptr;
		HANDLE m_port = INVALID_HANDLE_VALUE;
		SOCKET m_listenSocket = INVALID_SOCKET;
		Map<Op*, Unique<Op>> m_scheduledOperations;
		Set<Unique<Connection>> m_connections;

		void pushPendingOp(Unique<Op> op)
		{
			m_scheduledOperations.insert(op.get(), std::move(op));
		}

		Unique<Op> popPendingOp(Op* op)
		{
			auto it = m_scheduledOperations.lookup(op);
			if (it == m_scheduledOperations.end()) return nullptr;
			auto res = std::move(it->value);
			m_scheduledOperations.remove(op);
			return res;
		}

		HumanError scheduleAccept()
		{
			auto op = unique_from<AcceptOp>(m_allocator);
			// TODO: we can receive on accept operation, it's a little bit more efficient so consider doing that in the future
			DWORD bytesReceived = 0;
			auto res = m_acceptFn(
				m_listenSocket, // listen socket
				op->acceptSocket(), // accept socket
				op->buffer(), // pointer to a buffer that receives the first block of data sent on new connection
				0, // number of bytes for receive data (it doesn't include server and client address)
				sizeof(SOCKADDR_IN) + 16, // local address length
				sizeof(SOCKADDR_IN) + 16, // remote address length
				&bytesReceived, // bytes received in case of sync completion
				(OVERLAPPED*)op.get() // overlapped structure
			);
			if (res == TRUE)
			{
				return handleAccept(std::move(op));
			}
			else
			{
				auto error = WSAGetLastError();
				if (error == ERROR_IO_PENDING)
				{
					pushPendingOp(std::move(op));
				}
				else
				{
					return errf(m_allocator, "Failed to schedule accept operation: ErrorCode({})"_sv, error);
				}
			}

			return {};
		}

		HumanError handleAccept(Unique<AcceptOp> op)
		{
			auto connection = core::unique_from<Connection>(m_allocator, op->acceptSocket());
			if (auto err = scheduleRead(connection.get())) return err;
			m_connections.insert(std::move(connection));
			return {};
		}

		HumanError scheduleRead(Connection* conn)
		{
			auto op = unique_from<ReadOp>(m_allocator, conn);

			WSABUF buf;
			buf.buf = conn->readBuffer();
			buf.len = conn->readBufferSize();
			auto res = WSARecv(
				conn->socket(),
				&buf,
				1,
				&op->m_bytesReceived,
				0,
				(OVERLAPPED*)op.get(),
				NULL
			);
			if (res == TRUE)
			{
				return handleRead(std::move(op));
			}
			else
			{
				auto err = WSAGetLastError();
				if (err == ERROR_IO_PENDING)
				{
					pushPendingOp(std::move(op));
				}
				else
				{
					return errf(m_allocator, "Failed to schedule read operation: ErrorCode({})"_sv, err);
				}
			}

			return {};
		}

		HumanError handleRead(Unique<ReadOp> op)
		{
			return {};
		}

	public:
		WinOSServer(HANDLE port, SOCKET listenSocket, LPFN_ACCEPTEX acceptEx, Allocator* allocator)
			: m_allocator(allocator),
			  m_acceptFn(acceptEx),
			  m_port(port),
			  m_listenSocket(listenSocket),
			  m_scheduledOperations(allocator),
			  m_connections(allocator)
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

				auto op = popPendingOp((Op*)overlapped);
				assert(op != nullptr);

				switch (op->kind())
				{
				case Op::KIND_ACCEPT:
					if (auto err = handleAccept(unique_static_cast<AcceptOp>(std::move(op)))) return err;
					break;
				case Op::KIND_READ:
					if (auto err = handleRead(unique_static_cast<ReadOp>(std::move(op)))) return err;
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

		PADDRINFOA addressInfo = nullptr;
		auto res = getaddrinfo("127.0.0.1", "123", nullptr, &addressInfo);
		if (res != 0)
			return nullptr;
			// return errf(allocator, "failed to get address info, ErrorCode({})"_sv, res);

		res = bind(listenSocket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen);
		freeaddrinfo(addressInfo);
		if (res != 0)
			return nullptr;

		res = listen(listenSocket, SOMAXCONN);
		if (res != 0)
			return nullptr;

		LPFN_ACCEPTEX acceptEx = nullptr;
		GUID guidAcceptEx = WSAID_ACCEPTEX;
		DWORD bytesReturned = 0;
		res = WSAIoctl(
			listenSocket,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidAcceptEx,
			sizeof(guidAcceptEx),
			&acceptEx,
			sizeof(acceptEx),
			&bytesReturned,
			NULL,
			NULL
		);

		if (res == SOCKET_ERROR)
			return nullptr;

		return core::unique_from<WinOSServer>(allocator, port, listenSocket, acceptEx, allocator);
	}
}