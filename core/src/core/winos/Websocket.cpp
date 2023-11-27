#include "core/Websocket.h"
#include "core/Hash.h"
#include "core/SHA1.h"
#include "core/Base64.h"
#include "core/Queue.h"

#include <tracy/Tracy.hpp>

#include <Windows.h>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

#include <cassert>

namespace core::websocket
{
	constexpr static uint8_t EMPTY_PONG[] = {128 | FrameHeader::OPCODE_PONG, 0x00};
	// code: 1000
	constexpr static uint8_t CLOSE_NORMAL[] = {128 | FrameHeader::OPCODE_CLOSE, 0x02, 0x03, 0xE8};
	// code: 1002
	constexpr static uint8_t CLOSE_PROTOCOL_ERROR[] = {128 | FrameHeader::OPCODE_CLOSE, 0x02, 0x03, 0xEA};

	class WinOSServer;

	struct WinOSConnection: public Connection
	{
		enum STATE
		{
			STATE_NONE,
			STATE_HANDSHAKE,
			STATE_READ_MESSAGE,
		};

		WinOSConnection(WinOSServer* server, SOCKET socket_, Allocator* allocator)
			: m_server(server),
			  socket(socket_),
			  handshakeBuffer(allocator),
			  handshake(allocator),
			  frameBuffer(allocator),
			  frameParser(allocator),
			  writeQueue(allocator)
		{
			::memset(readBuffer, 0, sizeof(readBuffer));
			handshakeBuffer.reserve(1 * 1024);
		}

		~WinOSConnection()
		{
			if (socket != INVALID_SOCKET)
			{
				[[maybe_unused]] auto res = ::closesocket(socket);
				assert(res == 0);
			}
		}

		HumanError writeRaw(Buffer&& bytes);
		HumanError writeRaw(StringView str);
		HumanError writeRaw(Span<const std::byte> bytes);

		HumanError writeFrameHeader(FrameHeader::OPCODE opcode, size_t payloadLength)
		{
			std::byte buf[10];
			buf[0] = std::byte(128 | opcode);

			if (payloadLength <= 125)
			{
				buf[1] = std::byte(payloadLength);
				if (auto err = writeRaw(Span<const std::byte>{buf, 2})) return err;
			}
			else if (payloadLength < UINT16_MAX)
			{
				buf[1] = std::byte(126);
				buf[2] = std::byte((payloadLength >> 8) & 0xFF);
				buf[3] = std::byte(payloadLength & 0xFF);
				if (auto err = writeRaw(Span<const std::byte>{buf, 4})) return err;
			}
			else
			{
				buf[1] = std::byte(127);
				buf[2] = std::byte((payloadLength >> 56) & 0xFF);
				buf[3] = std::byte((payloadLength >> 48) & 0xFF);
				buf[4] = std::byte((payloadLength >> 40) & 0xFF);
				buf[5] = std::byte((payloadLength >> 32) & 0xFF);
				buf[6] = std::byte((payloadLength >> 24) & 0xFF);
				buf[7] = std::byte((payloadLength >> 16) & 0xFF);
				buf[8] = std::byte((payloadLength >> 8) & 0xFF);
				buf[9] = std::byte(payloadLength & 0xFF);
				if (auto err = writeRaw(Span<const std::byte>{buf, 10})) return err;
			}

			return {};
		}

		HumanError writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> data)
		{
			if (auto err = writeFrameHeader(opcode, data.count())) return err;
			if (auto err = writeRaw(data)) return err;
			return {};
		}

		HumanError writeFrame(FrameHeader::OPCODE opcode, StringView data)
		{
			if (auto err = writeFrameHeader(opcode, data.count())) return err;
			if (auto err = writeRaw(data)) return err;
			return {};
		}

		HumanError writeFrame(FrameHeader::OPCODE opcode, Buffer&& data)
		{
			if (auto err = writeFrameHeader(opcode, data.count())) return err;
			if (auto err = writeRaw(std::move(data))) return err;
			return {};
		}

		HumanError writeText(StringView str) override
		{
			return writeFrame(FrameHeader::OPCODE_TEXT, Span<const std::byte>{str});
		}

		HumanError writeText(Buffer&& str) override
		{
			return writeFrame(FrameHeader::OPCODE_TEXT, std::move(str));
		}

		HumanError writeBinary(Span<const std::byte> bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_BINARY, bytes);
		}

		HumanError writeBinary(Buffer&& bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_BINARY, std::move(bytes));
		}

		HumanError writePing(Span<const std::byte> bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_PING, bytes);
		}

		HumanError writePing(Buffer&& bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_PING, std::move(bytes));
		}

		HumanError writePong(Span<const std::byte> bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_PONG, bytes);
		}

		HumanError writePong(Buffer&& bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_PONG, std::move(bytes));
		}

		HumanError writeClose() override
		{
			return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});
		}

		HumanError writeClose(uint16_t code) override
		{
			uint8_t buf[2];
			buf[0] = (code >> 8) & 0xFF;
			buf[1] = code & 0xFF;
			return writeFrame(FrameHeader::OPCODE_CLOSE, Span<const std::byte>{(const std::byte*)buf, 2});
		}


		WinOSServer* m_server = nullptr;
		STATE state = STATE_NONE;
		SOCKET socket = INVALID_SOCKET;
		CHAR readBuffer[1 * 1024];
		Buffer handshakeBuffer;
		Handshake handshake;
		Buffer frameBuffer;
		FrameParser frameParser;
		Queue<Buffer> writeQueue;
		bool isWriting = false;
	};

	class WinOSServer: public Server
	{
		struct Op: OVERLAPPED
		{
			enum KIND
			{
				KIND_NONE,
				KIND_ACCEPT,
				KIND_READ,
				KIND_WRITE,
			};

			Op(KIND kind_)
				: kind(kind_)
			{
				memset((OVERLAPPED*)this, 0, sizeof(OVERLAPPED));
				hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			}

			virtual ~Op()
			{
				CloseHandle(hEvent);
			}

			KIND kind = KIND_NONE;
		};

		struct AcceptOp: public Op
		{
			SOCKET acceptSocket = INVALID_SOCKET;
			std::byte buffer[2 * (sizeof(SOCKADDR_IN) + 16)];

			AcceptOp()
				: Op(KIND_ACCEPT)
			{
				acceptSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			}

			~AcceptOp()
			{
				if (acceptSocket != INVALID_SOCKET)
				{
					[[maybe_unused]] auto res = ::closesocket(acceptSocket);
					assert(res == 0);
				}
			}
		};

		struct ReadOp: public Op
		{
			WinOSConnection* connection = nullptr;
			DWORD bytesReceived = 0;
			DWORD flags = 0;
			WSABUF recvBuf{};

			ReadOp(WinOSConnection* connection_)
				: Op(KIND_READ),
				  connection(connection_)
			{
				recvBuf.buf = connection->readBuffer;
				recvBuf.len = sizeof(connection->readBuffer);
			}
		};

		struct WriteOp: public Op
		{
			WinOSConnection* connection = nullptr;
			DWORD bytesSent = 0;
			Buffer buffer;
			WSABUF wsaBuf{};
			DWORD flags = 0;

			WriteOp(WinOSConnection* connection_, Buffer&& buffer_)
				: Op(KIND_WRITE),
				  connection(connection_),
				  buffer(std::move(buffer_))
			{
				wsaBuf.buf = (CHAR*)buffer.data();
				wsaBuf.len = (ULONG)buffer.count();
			}
		};

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		LPFN_ACCEPTEX m_acceptFn = nullptr;
		HANDLE m_port = INVALID_HANDLE_VALUE;
		SOCKET m_listenSocket = INVALID_SOCKET;
		Map<Op*, Unique<Op>> m_scheduledOperations;
		Set<Unique<WinOSConnection>> m_connections;
		Handler* m_handler = nullptr;
		bool running = false;

		void pushPendingOp(Unique<Op> op)
		{
			ZoneScoped;

			m_scheduledOperations.insert(op.get(), std::move(op));
		}

		Unique<Op> popPendingOp(Op* op)
		{
			ZoneScoped;

			auto it = m_scheduledOperations.lookup(op);
			if (it == m_scheduledOperations.end())
				return nullptr;
			auto res = std::move(it->value);
			m_scheduledOperations.remove(op);
			return res;
		}

		HumanError onTextMsg(const Msg& msg, WinOSConnection* conn)
		{
			if (m_handler->onMsg)
			{
				ZoneScoped;
				return m_handler->onMsg(msg, conn);
			}
			return {};
		}

		HumanError onBinaryMsg(const Msg& msg, WinOSConnection* conn)
		{
			if (m_handler->onMsg)
			{
				ZoneScoped;
				return m_handler->onMsg(msg, conn);
			}
			return {};
		}

		HumanError onCloseMsg(const Msg& msg, WinOSConnection* conn)
		{
			if (m_handler->handleClose)
			{
				ZoneScoped;
				return m_handler->onMsg(msg, conn);
			}
			else
			{
				if (msg.payload.count() == 0)
					return conn->writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});

				// error protocol close payload should have at least 2 byte
				if (msg.payload.count() == 1)
					return conn->writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});

				auto errorCode = uint16_t(msg.payload[1]) | (uint16_t(msg.payload[0]) << 8);
				if (errorCode < 1000 || errorCode == 1004 || errorCode == 1005 || errorCode == 1006 || (errorCode > 1013  && errorCode < 3000))
				{
					return conn->writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});
				}

				if (msg.payload.count() == 2)
					return conn->writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});

				return conn->writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});
			}
		}

		HumanError onPingMsg(const Msg& msg, WinOSConnection* conn)
		{
			if (m_handler->handlePing)
			{
				ZoneScoped;
				return m_handler->onMsg(msg, conn);
			}
			else
			{
				if (msg.payload.count() == 0)
					return conn->writePong(
						Span<const std::byte>{(const std::byte *) EMPTY_PONG, sizeof(EMPTY_PONG)}
					);
				else
					return conn->writePong(msg.payload);
			}
		}

		HumanError onPongMsg(const Msg& msg, WinOSConnection* conn)
		{
			if (m_handler->handlePong)
			{
				ZoneScoped;
				return m_handler->onMsg(msg, conn);
			}
			return {};
		}

		HumanError onMsg(const Msg& msg, WinOSConnection* conn)
		{
			switch (msg.type)
			{
			case Msg::TYPE_TEXT: return onTextMsg(msg, conn);
			case Msg::TYPE_BINARY: return onBinaryMsg(msg, conn);
			case Msg::TYPE_CLOSE: return onCloseMsg(msg, conn);
			case Msg::TYPE_PING: return onPingMsg(msg, conn);
			case Msg::TYPE_PONG: return onPongMsg(msg, conn);
			default:
				assert(false);
				return errf(m_allocator, "Invalid message type"_sv);
			}
		}

		HumanError scheduleAccept()
		{
			ZoneScoped;

			if (running == false)
				return {};

			auto op = unique_from<AcceptOp>(m_allocator);
			// TODO: we can receive on accept operation, it's a little bit more efficient so consider doing that in the future
			DWORD bytesReceived = 0;
			auto res = m_acceptFn(
				m_listenSocket, // listen socket
				op->acceptSocket, // accept socket
				op->buffer, // pointer to a buffer that receives the first block of data sent on new connection
				0, // number of bytes for receive data (it doesn't include server and client address)
				sizeof(SOCKADDR_IN) + 16, // local address length
				sizeof(SOCKADDR_IN) + 16, // remote address length
				&bytesReceived, // bytes received in case of sync completion
				(OVERLAPPED*)op.get() // overlapped structure
			);
			if (res == TRUE || WSAGetLastError() == ERROR_IO_PENDING)
			{
				pushPendingOp(std::move(op));
			}
			else
			{
				auto error = WSAGetLastError();
				return errf(m_allocator, "Failed to schedule accept operation: ErrorCode({})"_sv, error);
			}

			return {};
		}

		HumanError handleAccept(Unique<AcceptOp> op)
		{
			ZoneScoped;

			auto res = CreateIoCompletionPort((HANDLE)op->acceptSocket, m_port, NULL, 0);
			assert(res == m_port);

			auto acceptSocket = op->acceptSocket;
			op->acceptSocket = INVALID_SOCKET;

			auto connection = core::unique_from<WinOSConnection>(m_allocator, this, acceptSocket, m_allocator);
			connection->state = WinOSConnection::STATE_HANDSHAKE;
			if (auto err = scheduleRead(connection.get())) return err;
			m_connections.insert(std::move(connection));
			return {};
		}

		HumanError scheduleRead(WinOSConnection* conn)
		{
			ZoneScoped;

			if (running == false)
				return {};

			auto op = unique_from<ReadOp>(m_allocator, conn);

			auto res = WSARecv(
				conn->socket,
				&op->recvBuf,
				1,
				&op->bytesReceived,
				&op->flags,
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
				return errf(m_allocator, "Failed to schedule read operation: ErrorCode({})"_sv, error);
			}

			return {};
		}

		HumanError handleRead(Unique<ReadOp> op)
		{
			ZoneScoped;

			auto conn = op->connection;

			switch (conn->state)
			{
			case WinOSConnection::STATE_NONE: return errf(m_allocator, "Connection in none state"_sv);
			case WinOSConnection::STATE_HANDSHAKE:
			{
				ZoneScopedN("handshake");
				auto totalHandshakeBuffer = conn->handshakeBuffer.count() + op->bytesReceived;
				if (totalHandshakeBuffer > conn->handshakeBuffer.capacity())
				{
					(void)scheduleWrite(conn, R"(HTTP/1.1 400 Invalid\r\nerror: too large\r\ncontent-length: 0\r\n\r\n)"_sv);
					return errf(m_allocator, "Handshake buffer overflow, max handshake buffer size is {} bytes, but {} bytes is needed"_sv, totalHandshakeBuffer, conn->handshakeBuffer.capacity());
				}

				conn->handshakeBuffer.push((const std::byte*)op->recvBuf.buf, op->bytesReceived);

				auto handshakeString = StringView{conn->handshakeBuffer};
				if (handshakeString.endsWith("\r\n\r\n"_sv))
				{
					auto handshakeResult = Handshake::parse(handshakeString, m_allocator);
					if (handshakeResult.isError())
					{
						(void)scheduleWrite(conn, R"(HTTP/1.1 400 Invalid\r\nerror: failed to parse handshake\r\ncontent-length: 0\r\n\r\n)"_sv);
						return errf(m_allocator, "Failed to parse handshake: {}"_sv, handshakeResult.error());
					}
					conn->handshake = std::move(handshakeResult.value());
					conn->handshakeBuffer = Buffer{m_allocator};

					m_log->debug("handshake: {}"_sv, conn->handshake.key());

					constexpr static const char* REPLY = "HTTP/1.1 101 Switching Protocols\r\n"
						"Upgrade: websocket\r\n"
						"Connection: Upgrade\r\n"
						"Sec-WebSocket-Accept: {}\r\n"
						"\r\n";
					auto concatKey = strf(m_allocator, "{}{}"_sv, conn->handshake.key(), "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
					auto sha1 = SHA1::hash(concatKey);
					auto base64 = Base64::encode(sha1.asBytes(), m_allocator);
					auto reply = strf(m_allocator, StringView{REPLY, strlen(REPLY)}, base64);
					if (auto err = scheduleWrite(conn, reply)) return err;

					// move to the read message state
					conn->state = WinOSConnection::STATE_READ_MESSAGE;
					if (auto err = scheduleRead(conn)) return err;
				}
				else
				{
					// schedule another read operation to continue reading the handshake
					if (auto err = scheduleRead(conn)) return err;
				}

				return {};
			}
			case WinOSConnection::STATE_READ_MESSAGE:
			{
				ZoneScopedN("read message");

				conn->frameBuffer.push((const std::byte*)op->recvBuf.buf, op->bytesReceived);

				while (conn->frameParser.neededBytes() <= conn->frameBuffer.count())
				{
					auto parserResult = conn->frameParser.consume(conn->frameBuffer.data(), conn->frameBuffer.count());
					if (parserResult.isError()) return parserResult.releaseError();

					// we have reached the end of a frame
					if (parserResult.value())
					{
						ZoneScopedN("websocket frame complete");
						auto msg = conn->frameParser.msg();
						if (auto err = onMsg(msg, conn)) return err;

						if (conn->frameBuffer.count() > conn->frameParser.neededBytes())
						{
							::memcpy(conn->frameBuffer.data(), conn->frameBuffer.data() + conn->frameParser.neededBytes(), conn->frameBuffer.count() - conn->frameParser.neededBytes());
							conn->frameBuffer.resize(conn->frameBuffer.count() - conn->frameParser.neededBytes());
						}
						else
						{
							conn->frameBuffer.clear();
						}

						// reset parser state
						conn->frameParser = FrameParser{m_allocator};
					}
				}

				if (auto err = scheduleRead(conn)) return err;
				return {};
			}
			default:
				assert(false);
				return errf(m_allocator, "Invalid connection state"_sv);
			}
		}

		HumanError scheduleWriteOp(Unique<WriteOp> op)
		{
			ZoneScoped;

			if (running == false)
				return {};

			auto conn = op->connection;

			auto res = WSASend(
				conn->socket,
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

		HumanError processWriteQueue(WinOSConnection* conn)
		{
			ZoneScoped;

			if (conn->writeQueue.count() == 0)
				return {};

			if (conn->isWriting)
				return {};

			auto buffer = std::move(conn->writeQueue.front());
			conn->writeQueue.pop_front();
			auto op = unique_from<WriteOp>(m_allocator, conn, std::move(buffer));
			return scheduleWriteOp(std::move(op));
		}

		HumanError handleWrite(Unique<WriteOp> op)
		{
			ZoneScoped;

			if (op->bytesSent == 0)
			{
				return errf(m_allocator, "Failed to send data"_sv);
			}
			else if (op->bytesSent == op->wsaBuf.len)
			{
				auto conn = op->connection;
				conn->isWriting = false;
				// advance the write queue
				return processWriteQueue(op->connection);
			}
			else if (op->bytesSent < op->wsaBuf.len)
			{
				op->wsaBuf.buf += op->bytesSent;
				op->wsaBuf.len -= op->bytesSent;
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

		HumanError scheduleWrite(WinOSConnection* conn, Buffer&& buffer)
		{
			ZoneScoped;

			conn->writeQueue.push_back(std::move(buffer));
			return processWriteQueue(conn);
		}

		HumanError scheduleWrite(WinOSConnection* conn, StringView str)
		{
			ZoneScoped;

			Buffer buffer{m_allocator};
			buffer.push(str);
			conn->writeQueue.push_back(std::move(buffer));
			return processWriteQueue(conn);
		}

		HumanError scheduleWrite(WinOSConnection* conn, Span<const std::byte> bytes)
		{
			ZoneScoped;

			Buffer buffer{m_allocator};
			buffer.push(bytes);
			conn->writeQueue.push_back(std::move(buffer));
			return processWriteQueue(conn);
		}

		HumanError run(Handler* handler) override
		{
			ZoneScoped;

			m_handler = handler;
			running = true;

			// schedule an accept operation
			if (auto err = scheduleAccept()) return err;

			while (running || m_scheduledOperations.count() > 1)
			{
				constexpr int MAX_ENTRIES = 32;
				OVERLAPPED_ENTRY entries[MAX_ENTRIES];
				ULONG numEntries = 0;
				{
					ZoneScopedN("GetQueuedCompletionStatus");

					auto res = GetQueuedCompletionStatusEx(m_port, entries, MAX_ENTRIES, &numEntries, INFINITE, FALSE);
					if (res == FALSE)
					{
						auto error = GetLastError();
						return errf(m_allocator, "Failed to get queued completion status: ErrorCode({})"_sv, error);
					}
				}

				for (size_t i = 0; i < numEntries; ++i)
				{
					auto overlapped = entries[i].lpOverlapped;

					auto op = popPendingOp((Op *) overlapped);
					assert(op != nullptr);

					switch (op->kind)
					{
					case Op::KIND_ACCEPT:
						if (auto err = handleAccept(unique_static_cast<AcceptOp>(std::move(op)))) return err;
						if (auto err = scheduleAccept()) return err;
						break;
					case Op::KIND_READ:
					{
						auto readOp = unique_static_cast<ReadOp>(std::move(op));
						DWORD dwFlags = 0;
						auto res = WSAGetOverlappedResult(
							readOp->connection->socket,
							overlapped,
							&readOp->bytesReceived,
							FALSE,
							&dwFlags
						);
						if (res != TRUE)
						{
							return errf(
								m_allocator,
								"Failed to get overlapped result: ErrorCode({})"_sv,
								WSAGetLastError());
						}
						if (auto err = handleRead(std::move(readOp))) return err;
						break;
					}
					case Op::KIND_WRITE:
					{
						auto writeOp = unique_static_cast<WriteOp>(std::move(op));
						DWORD dwFlags = 0;
						auto res = WSAGetOverlappedResult(
							writeOp->connection->socket,
							overlapped,
							&writeOp->bytesSent,
							FALSE,
							&dwFlags
						);
						if (res != TRUE)
						{
							return errf(
								m_allocator,
								"Failed to get overlapped result: ErrorCode({})"_sv,
								WSAGetLastError());
						}
						if (auto err = handleWrite(std::move(writeOp))) return err;
						break;
					}
					default:
						assert(false);
						break;
					}
				}
			}

			m_connections.clear();
			for (auto& op: m_scheduledOperations)
			{
				if (op.value->kind == Op::KIND_ACCEPT)
				{
					auto acceptOp = unique_static_cast<AcceptOp>(std::move(op.value));
					CancelIoEx((HANDLE)acceptOp->acceptSocket, (OVERLAPPED*)acceptOp.get());
				}
			}
			m_scheduledOperations.clear();

			return {};
		}

		void stop() override
		{
			for (auto& conn : m_connections)
			{
				(void)conn->writeClose();
			}
			running = false;
		}
	};

	HumanError WinOSConnection::writeRaw(Buffer&& bytes)
	{
		ZoneScoped;

		if (bytes.count() == 0)
			return {};

		return m_server->scheduleWrite(this, std::move(bytes));
	}

	HumanError WinOSConnection::writeRaw(StringView str)
	{
		ZoneScoped;

		if (str.count() == 0)
			return {};

		return m_server->scheduleWrite(this, str);
	}

	HumanError WinOSConnection::writeRaw(Span<const std::byte> bytes)
	{
		ZoneScoped;

		if (bytes.count() == 0)
			return {};

		return m_server->scheduleWrite(this, bytes);
	}

	Result<Unique<Server>> Server::open(StringView ip, StringView port, Log* log, Allocator* allocator)
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