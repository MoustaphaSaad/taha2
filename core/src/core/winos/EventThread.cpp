#include "core/EventThread.h"
#include "core/Hash.h"
#include "core/Mutex.h"

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

namespace core
{
	class WinOSThreadPool: public EventThreadPool
	{
		struct Op: OVERLAPPED
		{
			enum KIND
			{
				KIND_NONE,
				KIND_CLOSE,
				KIND_SEND_EVENT,
				KIND_ACCEPT,
			};

			explicit Op(KIND kind_)
				: kind(kind_)
			{}

			virtual ~Op() = default;

			KIND kind = KIND_NONE;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(KIND_CLOSE)
			{}
		};

		struct SendEventOp: Op
		{
			SendEventOp(Unique<Event2> event_, EventThread* thread_)
				: Op(KIND_SEND_EVENT),
				  event(std::move(event_)),
				  thread(thread_)
			{}

			Unique<Event2> event;
			EventThread* thread = nullptr;
		};

		struct AcceptOp: Op
		{
			Unique<Socket> socket;
			std::byte buffer[2 * sizeof(SOCKADDR_IN) + 16] = {};
			Weak<EventThread> thread;

			AcceptOp(Unique<Socket> socket_, const Shared<EventThread>& thread_)
				: Op(KIND_ACCEPT),
				  socket(std::move(socket_)),
				  thread(thread_)
			{}
		};

		class OpSet
		{
			Mutex m_mutex;
			Map<OVERLAPPED*, Unique<Op>> m_ops;
			bool m_open = true;
		public:
			explicit OpSet(Allocator* allocator)
				: m_ops(allocator),
				  m_mutex(allocator)
			{}

			bool tryPush(Unique<Op> op)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);

				if (m_open == false)
					return false;

				auto handle = (OVERLAPPED*)op.get();
				m_ops.insert(handle, std::move(op));
				return true;
			}

			Unique<Op> pop(OVERLAPPED* op)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);

				auto it = m_ops.lookup(op);
				if (it == m_ops.end())
					return nullptr;
				auto res = std::move(it->value);
				m_ops.remove(op);
				return res;
			}

			void close()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_open = false;
			}

			void open()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_open = true;
			}

			void clear()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_ops.clear();
			}
		};

		Log* m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		OpSet m_ops;
		Map<EventThread*, Shared<EventThread>> m_threads;
		ThreadPool* m_threadPool = nullptr;
		LPFN_ACCEPTEX m_acceptEx = nullptr;

		void addThread(const Shared<EventThread>& thread) override
		{
			auto handle = thread.get();
			m_threads.insert(handle, thread);

			auto startEvent = unique_from<StartEvent>(m_allocator);
			[[maybe_unused]] auto err = sendEvent(std::move(startEvent), handle);
			assert(!err);
		}

	protected:
		HumanError sendEvent(Unique<Event2> event, EventThread* thread) override
		{
			auto op = unique_from<SendEventOp>(m_allocator, std::move(event), thread);
			auto overlapped = (LPOVERLAPPED)op.get();
			if (m_ops.tryPush(std::move(op)))
			{
				auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, overlapped);
				if (FAILED(res))
					return errf(m_allocator, "failed to send event to thread"_sv);
			}
			return {};
		}

	public:
		WinOSThreadPool(ThreadPool* threadPool, HANDLE completionPort, LPFN_ACCEPTEX acceptEx, Log* log, Allocator* allocator)
			: EventThreadPool(allocator),
			  m_log(log),
			  m_completionPort(completionPort),
			  m_ops(allocator),
			  m_threads(allocator),
			  m_threadPool(threadPool),
			  m_acceptEx(acceptEx)
		{}

		~WinOSThreadPool() override
		{
			if (m_completionPort != INVALID_HANDLE_VALUE)
			{
				[[maybe_unused]] auto res = CloseHandle(m_completionPort);
				assert(SUCCEEDED(res));
			}
		}

		HumanError run() override
		{
			m_ops.open();

			while (true)
			{
				constexpr int MAX_ENTRIES = 128;
				OVERLAPPED_ENTRY entries[MAX_ENTRIES];
				ULONG numEntries = 0;
				{
					auto res = GetQueuedCompletionStatusEx(m_completionPort, entries, MAX_ENTRIES, &numEntries, INFINITE, false);
					if (res == FALSE)
					{
						auto error = GetLastError();
						return errf(m_allocator, "Failed to get queued completion status: ErrorCode({})"_sv, error);
					}
				}

				for (size_t i = 0; i < numEntries; ++i)
				{
					auto overlapped = entries[i].lpOverlapped;

					auto op = m_ops.pop(overlapped);
					assert(op != nullptr);

					switch (op->kind)
					{
					case Op::KIND_CLOSE:
					{
						m_ops.clear();
						m_threads.clear();
						return {};
					}
					case Op::KIND_SEND_EVENT:
					{
						auto sendEventOp = unique_static_cast<SendEventOp>(std::move(op));
						if (m_threadPool)
						{
							auto thread = sendEventOp->thread->sharedFromThis();
							auto func = Func<void()>{[event = std::move(sendEventOp->event), thread]{
								thread->handle(event.get());
							}};
							thread->executionQueue()->push(m_threadPool, std::move(func));
						}
						else
						{
							sendEventOp->thread->handle(sendEventOp->event.get());
						}
						break;
					}
					case Op::KIND_ACCEPT:
					{
						auto acceptOp = unique_static_cast<AcceptOp>(std::move(op));
						auto thread = acceptOp->thread.lock();
						if (m_threadPool)
						{
							auto event = unique_from<AcceptEvent2>(m_allocator, std::move(acceptOp->socket));
							auto func = Func<void()>{[event = std::move(event), thread]{
								thread->handle(event.get());
							}};
							thread->executionQueue()->push(m_threadPool, std::move(func));
						}
						else
						{
							AcceptEvent2 event{std::move(acceptOp->socket)};
							thread->handle(&event);
						}
						break;
					}
					default:
					{
						assert(false);
						return errf(m_allocator, "unknown internal op kind, {}"_sv, (int)op->kind);
					}
					}
				}
			}
		}

		void stop() override
		{
			auto op = unique_from<CloseOp>(m_allocator);
			auto overlapped = (LPOVERLAPPED)op.get();
			if (m_ops.tryPush(std::move(op)))
			{
				[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, overlapped);
				assert(SUCCEEDED(res));
			}
			m_ops.close();
		}

		HumanError registerSocket(const Unique<Socket>& socket) override
		{
			auto newPort = CreateIoCompletionPort((HANDLE)socket->fd(), m_completionPort, NULL, 0);
			if (newPort != m_completionPort)
				return errf(m_allocator, "failed to register socket to IOCP instance"_sv);
			return {};
		}

		HumanError accept(const Unique<Socket>& socket, const Shared<EventThread>& thread) override
		{
			// TODO: get the family and type from the accepting socket
			auto acceptedSocket = Socket::open(m_allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
			auto acceptedSocketHandle = acceptedSocket->fd();
			auto op = unique_from<AcceptOp>(m_allocator, std::move(acceptedSocket), thread);

			if (m_ops.tryPush(std::move(op)))
			{
				auto res = m_acceptEx(
					(SOCKET) socket->fd(), // listen socket
					(SOCKET) acceptedSocketHandle, // accept socket
					op->buffer, // pointer to a buffer that receives the first block of data sent on new connection
					0, // number of bytes for receive data (it doesn't include server and client address)
					sizeof(SOCKADDR_IN) + 16, // local address length
					sizeof(SOCKADDR_IN) + 16, // remote address length
					NULL, // bytes received in case of sync completion
					(OVERLAPPED *) op.get() // overlapped structure
				);
				if (res == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
				{
					auto error = WSAGetLastError();
					return errf(m_allocator, "failed to schedule accept operation: ErrorCode({})"_sv, error);
				}
			}
			return {};
		}
	};

	Result<Unique<EventThreadPool>> EventThreadPool::create(ThreadPool* threadPool, Log *log, Allocator *allocator)
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

		return unique_from<WinOSThreadPool>(allocator, threadPool, completionPort, acceptEx, log, allocator);
	}
}