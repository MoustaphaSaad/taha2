#include "core/EventLoop.h"
#include "core/Hash.h"

#include <Windows.h>
#include <WinSock2.h>

namespace core
{
	class WinOSEventLoop;

	class EventSource
	{
	public:
		virtual ~EventSource() = default;

		virtual HumanError read(WinOSEventLoop* loop) = 0;
	};

	class WinOSEventLoop: public EventLoop
	{
		struct Op: OVERLAPPED
		{
			enum KIND
			{
				KIND_NONE,
				KIND_CLOSE,
				KIND_READ,
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
				assert(SUCCEEDED(res));
			}

			KIND kind;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(KIND_CLOSE)
			{}
		};

		struct ReadOp: Op
		{
			DWORD bytesReceived = 0;
			DWORD flags = 0;
			WSABUF wsaBuf{};

			explicit ReadOp(Span<std::byte> buffer)
				: Op(KIND_READ)
			{
				wsaBuf.buf = (CHAR*)buffer.data();
				wsaBuf.len = (ULONG)buffer.count();
			}
		};

		class WinOSSocketEventSource: public EventSource
		{
			Unique<Socket> m_socket;
			std::byte recvLine[2048];
		public:
			explicit WinOSSocketEventSource(Unique<Socket> socket)
				: m_socket(std::move(socket))
			{
				::memset(recvLine, 0, sizeof(recvLine));
			}

			HumanError read(WinOSEventLoop* loop) override
			{
				auto op = unique_from<ReadOp>(loop->m_allocator, Span<std::byte>{recvLine, sizeof(recvLine)});

				auto res = WSARecv(
					(SOCKET)m_socket->fd(),
					&op->wsaBuf,
					1,
					&op->bytesReceived,
					&op->flags,
					(OVERLAPPED*)op.get(),
					NULL
				);

				if (res != SOCKET_ERROR || WSAGetLastError() == ERROR_IO_PENDING)
				{
					loop->pushPendingOp(std::move(op));
				}
				else
				{
					auto error = WSAGetLastError();
					return errf(loop->m_allocator, "failed to schedule read operation: ErrorCode({})"_sv, error);
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
		Map<Op*, Unique<Op>> m_scheduledOperations;
	public:
		WinOSEventLoop(HANDLE completionPort, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_completionPort(completionPort),
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
						m_log->debug("closed event loop"_sv);
						m_scheduledOperations.clear();
						return {};
					case Op::KIND_READ:
						m_log->debug("read event loop"_sv);
						break;
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

			return unique_from<WinOSSocketEventSource>(m_allocator, std::move(socket));
		}

		HumanError read(EventSource* source) override
		{
			if (source == nullptr)
				return {};

			return source->read(this);
		}
	};

	Result<Unique<EventLoop>> EventLoop::create(Log* log, Allocator* allocator)
	{
		auto completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (completionPort == NULL)
			return errf(allocator, "failed to create completion port"_sv);

		return unique_from<WinOSEventLoop>(allocator, completionPort, log, allocator);
	}
}