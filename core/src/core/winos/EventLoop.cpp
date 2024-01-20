#include "core/EventLoop.h"
#include "core/Hash.h"
#include "core/Queue.h"

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

#include <tracy/Tracy.hpp>

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
				KIND_CANCELED,
				KIND_CLOSE,
				KIND_READ,
				KIND_ACCEPT,
				KIND_WRITE,
			};

			explicit Op(KIND kind_, HANDLE handle_, const Weak<WinOSEventSource>& source_)
				: kind(kind_),
				  handle(handle_),
				  source(source_)
			{
				::memset((OVERLAPPED*)this, 0, sizeof(OVERLAPPED));
				// hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			}

			virtual ~Op()
			{
				// [[maybe_unused]] auto res = CloseHandle(hEvent);
				// assert(SUCCEEDED(res));
			}

			KIND kind = KIND_NONE;
			HANDLE handle = INVALID_HANDLE_VALUE;
			Weak<WinOSEventSource> source;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(KIND_CLOSE, nullptr, nullptr)
			{}
		};

		struct ReadOp: Op
		{
			DWORD flags = 0;
			WSABUF wsaBuf{};
			Reactor* reactor = nullptr;

			ReadOp(HANDLE handle_, Span<std::byte> buffer, const Weak<WinOSEventSource>& source_, Reactor* reactor_)
				: Op(KIND_READ, handle_, source_),
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

			AcceptOp(HANDLE handle_, Unique<Socket> socket_, const Weak<WinOSEventSource>& source_, Reactor* reactor_)
				: Op(KIND_ACCEPT, handle_, source_),
				  socket(std::move(socket_)),
				  reactor(reactor_)
			{}
		};

		struct WriteOp: Op
		{
			Buffer buffer;
			WSABUF wsaBuf{};
			Reactor* reactor = nullptr;

			WriteOp(HANDLE handle_, Buffer&& buffer_, const Weak<WinOSEventSource>& source_, Reactor* reactor_)
				: Op(KIND_WRITE, handle_, source_),
				  buffer(std::move(buffer_)),
				  reactor(reactor_)
			{
				wsaBuf.buf = (CHAR*)buffer.data();
				wsaBuf.len = (ULONG)buffer.count();
			}
		};

		class WinOSEventSource: public EventSource, public SharedFromThis<WinOSEventSource>
		{
		public:
			WinOSEventSource(EventLoop* loop, Allocator* allocator)
				: EventSource(loop)
			{}

			virtual Result<Unique<Op>> read(WinOSEventLoop* loop, Reactor* reactor) = 0;
			virtual Result<Unique<Op>> write(WinOSEventLoop* loop, Reactor* reactor, Span<const std::byte> buffer) = 0;
			virtual Result<Unique<Op>> accept(WinOSEventLoop* loop, Reactor* reactor) = 0;
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
				m_socket->shutdown(Socket::SHUT_RDWR);
			}

			Result<Unique<Op>> scheduleWriteOp(WinOSEventLoop* loop, Unique<WriteOp> op)
			{
				auto res = WSASend(
					(SOCKET)m_socket->fd(),
					&op->wsaBuf,
					1,
					NULL,
					0,
					(OVERLAPPED*)op.get(),
					NULL
				);

				if (res != SOCKET_ERROR || WSAGetLastError() == ERROR_IO_PENDING)
				{
					return std::move(op);
				}
				else
				{
					auto error = WSAGetLastError();
					return errf(loop->m_allocator, "failed to schedule write operation: ErrorCode({})"_sv, error);
				}
			}

			Result<Unique<Op>> read(WinOSEventLoop* loop, Reactor* reactor) override
			{
				auto op = unique_from<ReadOp>(loop->m_allocator, (HANDLE)m_socket->fd(), Span<std::byte>{recvLine, sizeof(recvLine)}, weakFromThis(), reactor);

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
					return std::move(op);
				}
				else
				{
					auto error = WSAGetLastError();
					return errf(loop->m_allocator, "failed to schedule read operation: ErrorCode({})"_sv, error);
				}
			}

			Result<Unique<Op>> write(WinOSEventLoop* loop, Reactor* reactor, Span<const std::byte> bytes) override
			{
				Buffer buffer{loop->m_allocator};
				buffer.push(bytes);
				auto op = unique_from<WriteOp>(loop->m_allocator, (HANDLE)m_socket->fd(), std::move(buffer), weakFromThis(), reactor);
				return scheduleWriteOp(loop, std::move(op));
			}

			Result<Unique<Op>> accept(WinOSEventLoop* loop, Reactor* reactor) override
			{
				// TODO: get the family and type from the accepting socket
				auto socket = Socket::open(loop->m_allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
				auto handle = (HANDLE)socket->fd();

				auto op = unique_from<AcceptOp>(loop->m_allocator, handle, std::move(socket), weakFromThis(), reactor);

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
					return std::move(op);
				}
				else
				{
					auto error = WSAGetLastError();
					return errf(loop->m_allocator, "failed to schedule accept operation: ErrorCode({})"_sv, error);
				}
			}
		};

		void pushPendingOp(Unique<Op> op)
		{
			auto handle = (OVERLAPPED*)op.get();
			m_scheduledOperations.insert(handle, std::move(op));
		}

		Unique<Op> popPendingOp(OVERLAPPED* op)
		{
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
		Map<OVERLAPPED*, Unique<Op>> m_scheduledOperations;
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
			ZoneScoped;
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
					auto overlappedOp = (Op*)overlapped;

					DWORD bytesTransferred = 0;
					GetOverlappedResult(overlappedOp->handle, overlapped, &bytesTransferred, FALSE);

					auto op = popPendingOp(overlapped);
					assert(op != nullptr);
					if (op->source && op->source.expired())
					{
						continue;
					}
					Shared<WinOSEventSource> source{};
					if (op->source)
						source = op->source.lock();

					switch (op->kind)
					{
					case Op::KIND_CANCELED:
					{
						break;
					}
					case Op::KIND_CLOSE:
					{
						ZoneScopedN("EventLoop::close");
						m_scheduledOperations.clear();
						return {};
					}
					case Op::KIND_READ:
					{
						auto readOp = unique_static_cast<ReadOp>(std::move(op));
						if (readOp->reactor)
						{
							ZoneScopedN("EventLoop::handle");
							ReadEvent readEvent{Span<const std::byte>{(const std::byte*)readOp->wsaBuf.buf, bytesTransferred}, source.get()};
							readOp->reactor->handle(&readEvent);
						}
						break;
					}
					case Op::KIND_ACCEPT:
					{
						auto acceptOp = unique_static_cast<AcceptOp>(std::move(op));
						if (acceptOp->reactor)
						{
							ZoneScopedN("EventLoop::accept");
							AcceptEvent acceptEvent{std::move(acceptOp->socket), source.get()};
							acceptOp->reactor->handle(&acceptEvent);
						}
						break;
					}
					case Op::KIND_WRITE:
					{
						auto writeOp = unique_static_cast<WriteOp>(std::move(op));
						if (writeOp->buffer.count() != bytesTransferred)
							return errf(m_allocator, "{}/{} bytes delieverd, failed to deliver full async send"_sv, bytesTransferred, writeOp->buffer.count());

						if (writeOp->reactor)
						{
							ZoneScopedN("EventLoop::write");
							WriteEvent writeEvent{bytesTransferred, source.get()};
							writeOp->reactor->handle(&writeEvent);
						}
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
			ZoneScoped;
			auto op = unique_from<CloseOp>(m_allocator);
			[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, (LPOVERLAPPED)op.get());
			assert(SUCCEEDED(res));
			pushPendingOp(std::move(op));
		}

		Shared<EventSource> createEventSource(Unique<Socket> socket) override
		{
			ZoneScoped;
			if (socket == nullptr)
				return nullptr;

			auto newPort = CreateIoCompletionPort((HANDLE)socket->fd(), m_completionPort, NULL, 0);
			assert(newPort == m_completionPort);
			return shared_from<WinOSSocketEventSource>(m_allocator, std::move(socket), this, m_allocator);
		}

		HumanError read(EventSource* source, Reactor* reactor) override
		{
			ZoneScoped;
			if (source == nullptr)
				return {};

			auto winOSSource = (WinOSEventSource*)source;
			auto opResult = winOSSource->read(this, reactor);
			if (opResult.isError())
				return opResult.releaseError();
			pushPendingOp(opResult.releaseValue());
			return {};
		}

		HumanError write(EventSource* source, Reactor* reactor, Span<const std::byte> buffer) override
		{
			ZoneScoped;
			if (source == nullptr)
				return {};

			auto winOSSource = (WinOSEventSource*)source;
			auto opResult = winOSSource->write(this, reactor, buffer);
			if (opResult.isError())
				return opResult.releaseError();
			else
				pushPendingOp(opResult.releaseValue());
			return {};
		}


		HumanError accept(EventSource* source, Reactor* reactor) override
		{
			ZoneScoped;
			if (source == nullptr)
				return {};

			auto winOSSource = (WinOSEventSource*)source;
			auto opResult = winOSSource->accept(this, reactor);
			if (opResult.isError())
				return opResult.releaseError();
			pushPendingOp(opResult.releaseValue());
			return {};
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