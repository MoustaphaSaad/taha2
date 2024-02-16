#include "core/EventThread.h"
#include "core/Hash.h"
#include "core/Mutex.h"

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

#include <tracy/Tracy.hpp>

namespace core
{
	class WinOSThreadPool: public EventThreadPool
	{
		struct Op : OVERLAPPED
		{
			enum KIND
			{
				KIND_NONE,
				KIND_CLOSE,
				KIND_SEND_EVENT,
				KIND_ACCEPT,
				KIND_READ,
				KIND_WRITE,
				KIND_STOP_THREAD,
			};

			explicit Op(KIND kind_, HANDLE handle_)
				: kind(kind_),
				  handle(handle_)
			{
				::memset((OVERLAPPED*)this, 0, sizeof(OVERLAPPED));
			}

			virtual ~Op() = default;

			KIND kind = KIND_NONE;
			HANDLE handle = INVALID_HANDLE_VALUE;
		};

		struct CloseOp : Op
		{
			CloseOp()
				: Op(KIND_CLOSE, INVALID_HANDLE_VALUE)
			{}
		};

		struct SendEventOp : Op
		{
			SendEventOp(Unique<Event2> event_, const Shared<EventThread>& thread_)
				: Op(KIND_SEND_EVENT, INVALID_HANDLE_VALUE),
				  event(std::move(event_)),
				  thread(thread_)
			{}

			Unique<Event2> event;
			Weak<EventThread> thread;
		};

		struct AcceptOp : Op
		{
			Unique<Socket> socket;
			std::byte buffer[2 * (sizeof(SOCKADDR_IN) + 16)] = {};
			Weak<EventThread> thread;

			AcceptOp(Unique<Socket> socket_, const Shared<EventThread> &thread_, HANDLE handle_)
				: Op(KIND_ACCEPT, handle_),
				  socket(std::move(socket_)),
				  thread(thread_)
			{}
		};

		struct ReadOp : Op
		{
			std::byte buffer[1024] = {};
			DWORD flags = 0;
			WSABUF wsaBuf{};
			Weak<EventThread> thread;

			ReadOp(const Shared<EventThread> &thread_, HANDLE handle_)
				: Op(KIND_READ, handle_),
				  thread(thread_)
			{
				wsaBuf.buf = (CHAR*)buffer;
				wsaBuf.len = (ULONG)sizeof(buffer);
			}
		};

		struct WriteOp : Op
		{
			Buffer buffer;
			WSABUF wsaBuf{};
			Weak<EventThread> thread;

			WriteOp(const Shared<EventThread> &thread_, Buffer&& buffer_, HANDLE handle_)
				: Op(KIND_WRITE, handle_),
				  buffer(buffer_),
				  thread(thread_)
			{
				wsaBuf.buf = (CHAR*)buffer.data();
				wsaBuf.len = (ULONG)buffer.count();
			}
		};

		struct StopThreadOp : Op
		{
			Weak<EventThread> thread;

			explicit StopThreadOp(const Shared<EventThread> &thread_)
				: Op(KIND_STOP_THREAD, INVALID_HANDLE_VALUE),
				  thread(thread_)
			{}
		};

		class OpSet
		{
			Mutex m_mutex;
			Map<OVERLAPPED *, Unique<Op>> m_ops;
			bool m_open = true;
		public:
			explicit OpSet(Allocator *allocator)
					: m_ops(allocator),
					  m_mutex(allocator)
			{}

			bool tryPush(Unique<Op> op)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);

				if (m_open == false)
					return false;

				auto handle = (OVERLAPPED *) op.get();
				m_ops.insert(handle, std::move(op));
				return true;
			}

			Unique<Op> pop(OVERLAPPED *op)
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

		class ThreadSet
		{
			Mutex m_mutex;
			Map<EventThread *, Shared<EventThread>> m_threads;
		public:
			explicit ThreadSet(Allocator* allocator)
				: m_mutex(allocator),
				  m_threads(allocator)
			{}

			void push(const Shared<EventThread>& thread)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				auto handle = thread.get();
				m_threads.insert(handle, thread);
			}

			void pop(EventThread* handle)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_threads.remove(handle);
			}

			void clear()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_threads.clear();
			}
		};

		Log *m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		OpSet m_ops;
		ThreadSet m_threads;
		ThreadPool *m_threadPool = nullptr;
		Array<Thread> m_driverThreads;
		LPFN_ACCEPTEX m_acceptEx = nullptr;
		size_t m_driverThreadsCount = 1;
		std::atomic<int> m_runningDriverThreads = 0;

		void addThread(const Shared<EventThread> &thread) override
		{
			ZoneScoped;
			m_threads.push(thread);

			auto startEvent = unique_from<StartEvent>(m_allocator);
			[[maybe_unused]] auto err = sendEvent(std::move(startEvent), thread);
			assert(!err);
		}

		HumanError driveEventLoop()
		{
			m_runningDriverThreads.fetch_add(1);
			while (true)
			{
				constexpr int MAX_ENTRIES = 128;
				OVERLAPPED_ENTRY entries[MAX_ENTRIES];
				ULONG numEntries = 0;
				{
					ZoneScopedN("EventThreadPool::GetQueuedCompletionStatusEx");
					auto entriesSize = MAX_ENTRIES;
					if (m_driverThreadsCount > 1)
						entriesSize = 1;
					auto res = GetQueuedCompletionStatusEx(m_completionPort, entries, entriesSize, &numEntries,
														   INFINITE, false);
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

					auto op = m_ops.pop(overlapped);
					assert(op != nullptr);

					DWORD bytesTransferred = 0;
					if (op->handle != INVALID_HANDLE_VALUE)
						GetOverlappedResult(op->handle, overlapped, &bytesTransferred, FALSE);

					switch (op->kind)
					{
					case Op::KIND_CLOSE:
					{
						ZoneScopedN("EventThreadPool::Close");
						if (m_runningDriverThreads.fetch_sub(1) == 1)
						{
							m_ops.clear();
							m_threads.clear();
						}
						return {};
					}
					case Op::KIND_SEND_EVENT:
					{
						ZoneScopedN("EventThreadPool::SendEvent");
						auto sendEventOp = unique_static_cast<SendEventOp>(std::move(op));
						auto thread = sendEventOp->thread.lock();
						if (m_threadPool)
						{
							auto func = Func<void()>{m_allocator, [event = std::move(sendEventOp->event), thread]
							{
								thread->handle(event.get());
							}};
							thread->executionQueue()->push(m_threadPool, std::move(func));
						}
						else
						{
							thread->handle(sendEventOp->event.get());
						}
						break;
					}
					case Op::KIND_ACCEPT:
					{
						ZoneScopedN("EventThreadPool::Accept");
						auto acceptOp = unique_static_cast<AcceptOp>(std::move(op));
						auto thread = acceptOp->thread.lock();
						if (m_threadPool)
						{
							auto func = Func<void()>{m_allocator, [acceptOp = std::move(acceptOp), thread]
							{
								AcceptEvent2 event{std::move(acceptOp->socket)};
								thread->handle(&event);
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
					case Op::KIND_READ:
					{
						ZoneScopedN("EventThreadPool::Read");
						auto readOp = unique_static_cast<ReadOp>(std::move(op));
						auto thread = readOp->thread.lock();
						if (m_threadPool)
						{
							auto func = Func<void()>{m_allocator, [readOp = std::move(readOp), thread, bytesTransferred]
							{
								ReadEvent2 event{Span<const std::byte>{readOp->buffer, bytesTransferred}};
								thread->handle(&event);
							}};
							thread->executionQueue()->push(m_threadPool, std::move(func));
						}
						else
						{
							ReadEvent2 event{Span<const std::byte>{readOp->buffer, bytesTransferred}};
							thread->handle(&event);
						}
						break;
					}
					case Op::KIND_WRITE:
					{
						ZoneScopedN("EventThreadPool::Write");
						auto writeOp = unique_static_cast<WriteOp>(std::move(op));
						auto thread = writeOp->thread.lock();
						if (m_threadPool)
						{
							auto func = Func<void()>{m_allocator, [readOp = std::move(writeOp), thread, bytesTransferred]
							{
								WriteEvent2 event{bytesTransferred};
								thread->handle(&event);
							}};
							thread->executionQueue()->push(m_threadPool, std::move(func));
						}
						else
						{
							WriteEvent2 event{bytesTransferred};
							thread->handle(&event);
						}
						break;
					}
					case Op::KIND_STOP_THREAD:
					{
						ZoneScopedN("EventThreadPool::StopThread");
						auto stopThreadOp = unique_static_cast<StopThreadOp>(std::move(op));
						if (auto thread = stopThreadOp->thread.lock())
							m_threads.pop(thread.get());
						break;
					}
					default:
					{
						assert(false);
						return errf(m_allocator, "unknown internal op kind, {}"_sv, (int) op->kind);
					}
					}
				}
			}
		}

	protected:
		HumanError sendEvent(Unique<Event2> event, const Shared<EventThread>& thread) override
		{
			ZoneScoped;
			auto op = unique_from<SendEventOp>(m_allocator, std::move(event), thread);
			auto overlapped = (LPOVERLAPPED) op.get();
			if (m_ops.tryPush(std::move(op)))
			{
				auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, overlapped);
				if (FAILED(res))
					return errf(m_allocator, "failed to send event to thread"_sv);
			}
			return {};
		}

	public:
		WinOSThreadPool(size_t driverThreadsCount, ThreadPool *threadPool, HANDLE completionPort, LPFN_ACCEPTEX acceptEx, Log *log,
						Allocator *allocator)
				: EventThreadPool(allocator),
				  m_log(log),
				  m_completionPort(completionPort),
				  m_ops(allocator),
				  m_threads(allocator),
				  m_threadPool(threadPool),
				  m_driverThreads(allocator),
				  m_acceptEx(acceptEx),
				  m_driverThreadsCount(driverThreadsCount)
		{}

		~WinOSThreadPool() override
		{
			for (auto& thread: m_driverThreads)
				thread.join();

			if (m_completionPort != INVALID_HANDLE_VALUE)
			{
				[[maybe_unused]] auto res = CloseHandle(m_completionPort);
				assert(SUCCEEDED(res));
			}
		}

		HumanError run() override
		{
			m_ops.open();

			assert(m_driverThreadsCount < 128);
			HumanError errors[128];
			for (size_t i = 1; i < m_driverThreadsCount; ++i)
			{
				m_driverThreads.push(Thread{m_allocator, [this, &errors, i]{
					errors[i] = driveEventLoop();
				}});
			}
			errors[0] = driveEventLoop();

			for (size_t i = 0; i < m_driverThreadsCount; ++i)
				if (errors[i])
					return errors[i];

			return {};
		}

		void stop() override
		{
			ZoneScoped;
			for (int i = 0; i < m_driverThreadsCount; ++i)
			{
				auto op = unique_from<CloseOp>(m_allocator);
				auto overlapped = (LPOVERLAPPED) op.get();
				if (m_ops.tryPush(std::move(op)))
				{
					[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, overlapped);
					assert(SUCCEEDED(res));
				}
			}
			m_ops.close();
			for (auto& thread: m_driverThreads)
				thread.join();
			m_driverThreads.clear();
		}

		HumanError registerSocket(const Unique<Socket> &socket) override
		{
			ZoneScoped;
			auto newPort = CreateIoCompletionPort((HANDLE) socket->fd(), m_completionPort, NULL, 0);
			if (newPort != m_completionPort)
				return errf(m_allocator, "failed to register socket to IOCP instance"_sv);
			return {};
		}

		HumanError accept(const Unique<Socket> &socket, const Shared<EventThread> &thread) override
		{
			ZoneScoped;
			// TODO: get the family and type from the accepting socket
			auto acceptedSocket = Socket::open(m_allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
			auto acceptedSocketHandle = acceptedSocket->fd();
			auto op = unique_from<AcceptOp>(m_allocator, std::move(acceptedSocket), thread, (HANDLE)socket->fd());
			auto overlapped = (OVERLAPPED *)op.get();
			auto buffer = (PVOID)op->buffer;

			if (m_ops.tryPush(std::move(op)))
			{
				auto res = m_acceptEx(
						(SOCKET) socket->fd(), // listen socket
						(SOCKET) acceptedSocketHandle, // accept socket
						buffer, // pointer to a buffer that receives the first block of data sent on new connection
						0, // number of bytes for receive data (it doesn't include server and client address)
						sizeof(SOCKADDR_IN) + 16, // local address length
						sizeof(SOCKADDR_IN) + 16, // remote address length
						NULL, // bytes received in case of sync completion
						overlapped // overlapped structure
				);
				if (res == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
				{
					auto error = WSAGetLastError();
					return errf(m_allocator, "failed to schedule accept operation: ErrorCode({})"_sv, error);
				}
			}
			return {};
		}

		HumanError read(const Unique<Socket> &socket, const Shared<EventThread> &thread) override
		{
			ZoneScoped;
			auto op = unique_from<ReadOp>(m_allocator, thread, (HANDLE)socket->fd());
			auto wsaBuf = &op->wsaBuf;
			auto flags = &op->flags;
			auto overlapped = (OVERLAPPED *)op.get();

			if (m_ops.tryPush(std::move(op)))
			{
				auto res = WSARecv(
					(SOCKET)socket->fd(),
					wsaBuf,
					1,
					NULL,
					flags,
					overlapped,
					NULL
				);
				if (res == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
				{
					auto error = WSAGetLastError();
					return errf(m_allocator, "failed to schedule read operation: ErrorCode({})"_sv, error);
				}
			}
			return {};
		}

		HumanError write(const Unique<Socket>& socket, const Shared<EventThread>& thread, Span<const std::byte> bytes) override
		{
			ZoneScoped;
			Buffer buffer{m_allocator};
			buffer.push(bytes);
			auto op = unique_from<WriteOp>(m_allocator, thread, std::move(buffer), (HANDLE)socket->fd());
			auto wsaBuf = &op->wsaBuf;
			auto overlapped = (OVERLAPPED *)op.get();

			if (m_ops.tryPush(std::move(op)))
			{
				auto res = WSASend(
					(SOCKET)socket->fd(),
					wsaBuf,
					1,
					NULL,
					0,
					overlapped,
					NULL
				);
				if (res == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
				{
					auto error = WSAGetLastError();
					return errf(m_allocator, "failed to schedule write operation: ErrorCode({})"_sv, error);
				}
			}
			return {};
		}

		void stopThread(const Shared<EventThread>& thread) override
		{
			ZoneScoped;
			auto op = unique_from<StopThreadOp>(m_allocator, thread);
			auto overlapped = (OVERLAPPED *)op.get();

			if (m_ops.tryPush(std::move(op)))
			{
				[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, overlapped);
				assert(SUCCEEDED(res));
			}
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

		size_t driverThreadsCount = 1;
		if (threadPool)
		{
			// spawn a driver thread per 10 worker thread
			constexpr auto WORKER_THREAD_PER_DRIVER_THREAD = 10;
			auto threadsCountRoundedUp = ((threadPool->threadsCount() + WORKER_THREAD_PER_DRIVER_THREAD - 1) / WORKER_THREAD_PER_DRIVER_THREAD) * WORKER_THREAD_PER_DRIVER_THREAD;
			driverThreadsCount = threadsCountRoundedUp / WORKER_THREAD_PER_DRIVER_THREAD;
		}

		return unique_from<WinOSThreadPool>(allocator, driverThreadsCount, threadPool, completionPort, acceptEx, log, allocator);
	}
}