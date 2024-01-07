#include "core/EventLoop.h"
#include "core/Hash.h"

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

namespace core
{
	class WinOSEventLoop: public EventLoop
	{
		class WinOSEventSource;

		struct Op: OVERLAPPED
		{
			enum KIND
			{
				KIND_NONE,
				KIND_CLOSE,
				KIND_READ,
				KIND_ACCEPT,
			};

			explicit Op(KIND kind_, WinOSEventSource* source_)
				: kind(kind_),
				  source(source_)
			{
				::memset((OVERLAPPED*)this, 0, sizeof(OVERLAPPED));
				hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			}

			virtual ~Op()
			{
				[[maybe_unused]] auto res = CloseHandle(hEvent);
				assert(SUCCEEDED(res));
			}

			KIND kind = KIND_NONE;
			WinOSEventSource* source = nullptr;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(KIND_CLOSE, nullptr)
			{}
		};

		struct ReadOp: Op
		{
			HANDLE handle;
			DWORD flags = 0;
			WSABUF wsaBuf{};
			Reactor* reactor = nullptr;

			ReadOp(HANDLE handle_, Span<std::byte> buffer, WinOSEventSource* source_, Reactor* reactor_)
				: Op(KIND_READ, source_),
				  handle(handle_),
				  reactor(reactor_)
			{
				wsaBuf.buf = (CHAR*)buffer.data();
				wsaBuf.len = (ULONG)buffer.count();
			}
		};

		struct AcceptOp: Op
		{
			Unique<Socket> socket;
			std::byte buffer[2 * sizeof(SOCKADDR_IN) + 16] = {};
			Reactor* reactor = nullptr;

			AcceptOp(Unique<Socket> socket_, WinOSEventSource* source_, Reactor* reactor_)
				: Op(KIND_ACCEPT, source_),
				  socket(std::move(socket_)),
				  reactor(reactor_)
			{}
		};

		class WinOSEventSource: public EventSource
		{
			Map<Op*, Unique<Op>> m_scheduledOperations;
		public:
			WinOSEventSource(EventLoop* loop, Allocator* allocator)
				: EventSource(loop),
				  m_scheduledOperations(allocator)
			{}

			void pushPendingOp(Unique<Op> op)
			{
				m_scheduledOperations.insert(op.get(), std::move(op));
			}

			Unique<Op> popPendingOp(Op* op)
			{
				auto it = m_scheduledOperations.lookup(op);
				if (it == m_scheduledOperations.end())
					return nullptr;
				auto res = std::move(it->value);
				m_scheduledOperations.remove(op);
				return res;
			}

			virtual HumanError read(WinOSEventLoop* loop, Reactor* reactor) = 0;
			virtual HumanError accept(WinOSEventLoop* loop, Reactor* reactor) = 0;
		};

		class WinOSSocketEventSource: public WinOSEventSource
		{
			Unique<Socket> m_socket;
			std::byte recvLine[2048] = {};
		public:
			WinOSSocketEventSource(Unique<Socket> socket, EventLoop* loop, Allocator* allocator)
				: WinOSEventSource(loop, allocator),
				  m_socket(std::move(socket))
			{
				::memset(recvLine, 0, sizeof(recvLine));
			}

			~WinOSSocketEventSource() override
			{
				[[maybe_unused]] auto res = m_socket->shutdown(Socket::SHUT_RDWR);
				assert(res);
			}

			HumanError read(WinOSEventLoop* loop, Reactor* reactor) override
			{
				auto op = unique_from<ReadOp>(loop->m_allocator, (HANDLE)m_socket->fd(), Span<std::byte>{recvLine, sizeof(recvLine)}, this, reactor);

				auto res = WSARecv(
					(SOCKET)m_socket->fd(),
					&op->wsaBuf,
					1,
					NULL,
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
					return errf(loop->m_allocator, "failed to schedule read operation: ErrorCode({})"_sv, error);
				}

				return {};
			}

			HumanError accept(WinOSEventLoop* loop, Reactor* reactor) override
			{
				// TODO: get the family and type from the accepting socket
				auto socket = Socket::open(loop->m_allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);

				auto op = unique_from<AcceptOp>(loop->m_allocator, std::move(socket), this, reactor);

				DWORD bytesReceived = 0;
				auto res = loop->m_acceptEx(
					(SOCKET)m_socket->fd(), // listen socket
					(SOCKET)op->socket->fd(), // accept socket
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
					return errf(loop->m_allocator, "failed to schedule accept operation: ErrorCode({})"_sv, error);
				}
				return {};
			}
		};

		void pushPendingOp(Unique<Op> op)
		{
			m_scheduledOperations.insert(op.get(), std::move(op));
		}

		Unique<Op> popPendingOp(Op* op)
		{
			// check if this operation is related to a source
			if (op->source)
				return op->source->popPendingOp(op);

			// if not then this is a global operation
			auto it = m_scheduledOperations.lookup(op);
			if (it == m_scheduledOperations.end())
				return nullptr;
			auto res = std::move(it->value);
			m_scheduledOperations.remove(op);
			return res;
		}

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		LPFN_ACCEPTEX m_acceptEx = nullptr;
		Map<Op*, Unique<Op>> m_scheduledOperations;
	public:
		WinOSEventLoop(HANDLE completionPort, LPFN_ACCEPTEX acceptEx, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_completionPort(completionPort),
			  m_acceptEx(acceptEx),
			  m_scheduledOperations(allocator)
		{}

		~WinOSEventLoop() override
		{
			if (m_completionPort != INVALID_HANDLE_VALUE)
			{
				[[maybe_unused]] auto res = CloseHandle(m_completionPort);
				assert(SUCCEEDED(res));
			}
		}

		HumanError run() override
		{
			m_log->debug("started event loop"_sv);

			while (true)
			{
				constexpr int MAX_ENTRIES = 32;
				OVERLAPPED_ENTRY entries[MAX_ENTRIES];
				ULONG numEntries = 0;
				{
					auto res = GetQueuedCompletionStatusEx(m_completionPort, entries, MAX_ENTRIES, &numEntries, INFINITE, FALSE);
					if (res == FALSE)
					{
						auto error = GetLastError();
						return errf(m_allocator, "Failed to get queued completion status: ErrorCode({})"_sv, error);
					}
				}

				for (size_t i = 0; i < numEntries; ++i)
				{
					auto overlapped = entries[i].lpOverlapped;

					auto op = popPendingOp((Op*)overlapped);
					assert(op != nullptr);

					switch (op->kind)
					{
					case Op::KIND_CLOSE:
					{
						m_log->debug("closed event loop"_sv);
						m_scheduledOperations.clear();
						return {};
					}
					case Op::KIND_READ:
					{
						auto readOp = unique_static_cast<ReadOp>(std::move(op));
						if (readOp->reactor)
						{
							DWORD bytesReceived = 0;
							auto res = GetOverlappedResult(readOp->handle, readOp.get(), &bytesReceived, FALSE);
							assert(SUCCEEDED(res));
							ReadEvent readEvent{Span<const std::byte>{(const std::byte*)readOp->wsaBuf.buf, bytesReceived}, readOp->source};
							readOp->reactor->handle(&readEvent);
						}
						m_log->debug("read event loop"_sv);
						break;
					}
					case Op::KIND_ACCEPT:
					{
						auto acceptOp = unique_static_cast<AcceptOp>(std::move(op));
						if (acceptOp->reactor)
						{
							AcceptEvent acceptEvent{std::move(acceptOp->socket), acceptOp->source};
							acceptOp->reactor->handle(&acceptEvent);
						}
						m_log->debug("accept event loop"_sv);
						break;
					}
					default:
						return errf(m_allocator, "unknown operation kind, {}"_sv, (int)op->kind);
					}
				}
			}
		}

		void stop() override
		{
			auto op = unique_from<CloseOp>(m_allocator);
			[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, (LPOVERLAPPED)op.get());
			assert(SUCCEEDED(res));
			pushPendingOp(std::move(op));
		}

		Unique<EventSource> createEventSource(Unique<Socket> socket) override
		{
			if (socket == nullptr)
				return nullptr;

			auto newPort = CreateIoCompletionPort((HANDLE)socket->fd(), m_completionPort, NULL, 0);
			assert(newPort == m_completionPort);
			return unique_from<WinOSSocketEventSource>(m_allocator, std::move(socket), this, m_allocator);
		}

		HumanError read(EventSource* source, Reactor* reactor) override
		{
			if (source == nullptr)
				return {};

			auto winOSSource = (WinOSEventSource*)source;
			return winOSSource->read(this, reactor);
		}

		HumanError accept(EventSource* source, Reactor* reactor) override
		{
			if (source == nullptr)
				return {};

			auto winOSSource = (WinOSEventSource*)source;
			return winOSSource->accept(this, reactor);
		}
	};

	Result<Unique<EventLoop>> EventLoop::create(Log* log, Allocator* allocator)
	{
		auto completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (completionPort == NULL)
			return errf(allocator, "failed to create completion port"_sv);

		auto dummySocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (dummySocket == INVALID_SOCKET)
			return errf(allocator, "failed to create dummy socket"_sv);

		GUID guidAcceptEx = WSAID_ACCEPTEX;
		LPFN_ACCEPTEX acceptEx = nullptr;
		DWORD bytesReturned = 0;
		auto res = WSAIoctl(
			dummySocket,
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
			return errf(allocator, "failed to get AcceptEx function pointer, ErrorCode({})"_sv, res);

		return unique_from<WinOSEventLoop>(allocator, completionPort, acceptEx, log, allocator);
	}
}