#include "core/Websocket.h"
#include "core/Hash.h"

#include <Windows.h>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

#include <cassert>

namespace core
{
	class WinOSServer: public WebSocketServer
	{
		class Connection
		{
		public:
			enum STATE
			{
				STATE_NONE,
				STATE_HANDSHAKE,
			};

			Connection(SOCKET socket, Allocator* allocator)
				: m_socket(socket),
				  m_handshakeBuffer(allocator)
			{
				::memset(m_readBuffer, 0, sizeof(m_readBuffer));
				m_handshakeBuffer.reserve(1 * 1024);
			}

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

			STATE state() const
			{
				return m_state;
			}

			void setState(STATE state)
			{
				m_state = state;
			}

		private:
			friend class WinOSServer;

			STATE m_state = STATE_NONE;
			SOCKET m_socket = INVALID_SOCKET;
			CHAR m_readBuffer[1 * 1024];
			Buffer m_handshakeBuffer;
		};

		class Op: public OVERLAPPED
		{
		public:
			enum KIND
			{
				KIND_NONE,
				KIND_ACCEPT,
				KIND_READ,
				KIND_WRITE,
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
			friend class WinOSServer;

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

			SOCKET releaseAcceptSocket()
			{
				auto res = m_acceptSocket;
				m_acceptSocket = INVALID_SOCKET;
				return res;
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
			DWORD m_flags = 0;
			WSABUF m_recvBuf{};
		public:
			ReadOp(Connection* connection)
				: Op(KIND_READ),
				  m_connection(connection)
			{
				m_recvBuf.buf = connection->readBuffer();
				m_recvBuf.len = connection->readBufferSize();
			}

			DWORD bytesReceived() const
			{
				return m_bytesReceived;
			}

			WSABUF* recvBuf()
			{
				return &m_recvBuf;
			}

			Connection* connection() const
			{
				return m_connection;
			}
		};

		class WriteOp: public Op
		{
			friend class WinOSServer;

			Connection* m_connection = nullptr;
			DWORD m_bytesSent = 0;
			Buffer m_buffer;
			WSABUF m_wsaBuf{};
			DWORD m_flags = 0;
		public:
			WriteOp(Connection* connection, Buffer&& buffer)
				: Op(KIND_WRITE),
				  m_connection(connection),
				  m_buffer(std::move(buffer))
			{
				m_wsaBuf.buf = (CHAR*)m_buffer.data();
				m_wsaBuf.len = (ULONG)m_buffer.count();
			}

			WSABUF* sendBuf()
			{
				return &m_wsaBuf;
			}
		};

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
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
				op->m_acceptSocket, // accept socket
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
			auto res = CreateIoCompletionPort((HANDLE)op->m_acceptSocket, m_port, NULL, 0);
			assert(res == m_port);

			auto connection = core::unique_from<Connection>(m_allocator, op->releaseAcceptSocket(), m_allocator);
			connection->setState(Connection::STATE_HANDSHAKE);
			if (auto err = scheduleRead(connection.get())) return err;
			m_connections.insert(std::move(connection));
			return {};
		}

		HumanError scheduleRead(Connection* conn)
		{
			auto op = unique_from<ReadOp>(m_allocator, conn);

			auto res = WSARecv(
				conn->socket(),
				op->recvBuf(),
				1,
				&op->m_bytesReceived,
				&op->m_flags,
				(OVERLAPPED*)op.get(),
				NULL
			);
			if (res != SOCKET_ERROR)
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
			auto conn = op->connection();

			switch (conn->state())
			{
			case Connection::STATE_NONE: return errf(m_allocator, "Connection in none state"_sv);
			case Connection::STATE_HANDSHAKE:
			{
				auto totalHandshakeBuffer = conn->m_handshakeBuffer.count() + op->bytesReceived();
				if (totalHandshakeBuffer > conn->m_handshakeBuffer.capacity())
				{
					(void)scheduleWrite(conn, R"(HTTP/1.1 400 Invalid\r\nerror: too large\r\ncontent-length: 0\r\n\r\n)"_sv);
					return errf(m_allocator, "Handshake buffer overflow, max handshake buffer size is {} bytes, but {} bytes is needed"_sv, totalHandshakeBuffer, conn->m_handshakeBuffer.capacity());
				}

				conn->m_handshakeBuffer.push((const std::byte*)op->recvBuf()->buf, op->bytesReceived());

				auto handshakeString = StringView{conn->m_handshakeBuffer};
				if (handshakeString.endsWith("\r\n\r\n"_sv))
				{
					auto handshakeResult = Handshake::parse(handshakeString, m_allocator);
					if (handshakeResult.isError())
					{
						(void)scheduleWrite(conn, R"(HTTP/1.1 400 Invalid\r\nerror: failed to parse handshake\r\ncontent-length: 0\r\n\r\n)"_sv);
						return errf(m_allocator, "Failed to parse handshake: {}"_sv, handshakeResult.error());
					}
					m_log->debug("Handshake key: {}"_sv, handshakeResult.value().key());
				}
				else
				{
					// schedule another read operation to continue reading the handshake
					if (auto err = scheduleRead(conn)) return err;
				}

				return {};
			};
			default:
				assert(false);
				return errf(m_allocator, "Invalid connection state"_sv);
			}
		}

		HumanError scheduleWriteOp(Unique<WriteOp> op)
		{
			auto conn = op->m_connection;

			auto res = WSASend(
				conn->socket(),
				op->sendBuf(),
				1,
				&op->m_bytesSent,
				op->m_flags,
				(OVERLAPPED*)op.get(),
				NULL
			);
			if (res != SOCKET_ERROR)
			{
				return handleWrite(std::move(op));
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
					return errf(m_allocator, "Failed to schedule write operation: ErrorCode({})"_sv, err);
				}
			}

			return {};
		}

		HumanError scheduleWrite(Connection* conn, Buffer&& buffer)
		{
			auto op = unique_from<WriteOp>(m_allocator, conn, std::move(buffer));
			return scheduleWriteOp(std::move(op));
		}

		HumanError scheduleWrite(Connection* conn, StringView str)
		{
			Buffer buffer{m_allocator};
			buffer.push(str);
			auto op = unique_from<WriteOp>(m_allocator, conn, std::move(buffer));
			return scheduleWriteOp(std::move(op));
		}

		HumanError handleWrite(Unique<WriteOp> op)
		{
			if (op->m_bytesSent == 0)
			{
				return errf(m_allocator, "Failed to send data"_sv);
			}
			else if (op->m_bytesSent == op->m_wsaBuf.len)
			{
				return {};
			}
			else if (op->m_bytesSent < op->m_wsaBuf.len)
			{
				op->m_wsaBuf.buf += op->m_bytesSent;
				op->m_wsaBuf.len -= op->m_bytesSent;
				return scheduleWriteOp(std::move(op));
			}
			else
			{
				assert(false);
				return errf(m_allocator, "Invalid bytes sent!"_sv);
			}
		}

	public:
		WinOSServer(HANDLE port, SOCKET listenSocket, LPFN_ACCEPTEX acceptEx, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
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

		HumanError run() override
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
				{
					auto readOp = unique_static_cast<ReadOp>(std::move(op));
					DWORD dwFlags = 0;
					auto res = WSAGetOverlappedResult(readOp->connection()->socket(), overlapped, &readOp->m_bytesReceived, FALSE, &dwFlags);
					if (res != TRUE)
					{
						return errf(m_allocator, "Failed to get overlapped result: ErrorCode({})"_sv, WSAGetLastError());
					}
					if (auto err = handleRead(std::move(readOp))) return err;
					break;
				}
				default:
					assert(false);
					break;
				}
			}

			return {};
		}

		void stop() override
		{
			// TODO: Implement this function
		}
	};

	Result<Unique<WebSocketServer>> WebSocketServer::open(StringView ip, StringView port, Log* log, Allocator* allocator)
	{
		auto completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
		if (completionPort == NULL) return errf(allocator, "failed to create completion port"_sv);

		auto listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (listenSocket == INVALID_SOCKET) return errf(allocator, "failed to create listening socket"_sv);

		PADDRINFOA addressInfo = nullptr;
		String ipString{ip, allocator};
		String portString{port, allocator};
		auto res = getaddrinfo(ipString.data(), portString.data(), nullptr, &addressInfo);
		if (res != 0)
			return errf(allocator, "failed to get address info, ErrorCode({})"_sv, res);

		res = bind(listenSocket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen);
		freeaddrinfo(addressInfo);
		if (res != 0)
			return errf(allocator, "failed to bind listening socket, ErrorCode({})"_sv, res);

		res = listen(listenSocket, SOMAXCONN);
		if (res != 0)
			return errf(allocator, "failed to move listening socket to listen state, ErrorCode({})"_sv, res);

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
			return errf(allocator, "failed to get the AcceptEx function pointer, ErrorCode({})"_sv, res);

		auto newPort = CreateIoCompletionPort((HANDLE)listenSocket, completionPort, NULL, 0);
		assert(newPort == completionPort);

		return core::unique_from<WinOSServer>(allocator, completionPort, listenSocket, acceptEx, log, allocator);
	}
}