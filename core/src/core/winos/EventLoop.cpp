#include "core/EventLoop.h"
#include "core/ThreadedEventLoop.h"
#include "core/Mutex.h"
#include "core/Hash.h"

#include <tracy/Tracy.hpp>

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

namespace core
{
	class WinOSEventLoop: public EventLoop
	{
		struct Op: OVERLAPPED
		{
			Op(HANDLE handle_)
				: handle(handle_)
			{
				::memset((OVERLAPPED*)this, 0, sizeof(OVERLAPPED));
			}

			virtual ~Op() = default;

			HANDLE handle;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(INVALID_HANDLE_VALUE)
			{}
		};

		struct SendEventOp: Op
		{
			SendEventOp(Unique<Event> event_, const Weak<EventThread>& thread_)
				: Op(INVALID_HANDLE_VALUE),
				  event(std::move(event_)),
				  thread(thread_)
			{}

			Unique<Event> event;
			Weak<EventThread> thread;
		};

		struct StopThreadOp: Op
		{
			StopThreadOp(const Weak<EventThread>& thread_)
				: Op(INVALID_HANDLE_VALUE),
				  thread(thread_)
			{}

			Weak<EventThread> thread;
		};

		struct AcceptOp: Op
		{
			AcceptOp(Unique<Socket> acceptSocket_, const Weak<EventThread>& thread_)
				: Op(INVALID_HANDLE_VALUE),
				  acceptSocket(std::move(acceptSocket_)),
				  thread(thread_)
			{}

			Unique<Socket> acceptSocket;
			Weak<EventThread> thread;
			std::byte buffer[2 * (sizeof(SOCKADDR_IN) + 16)] = {};
		};

		struct ReadOp: Op
		{
			ReadOp(HANDLE handle_, const Weak<EventThread>& thread_)
				: Op(handle_),
				  thread(thread_)
			{
				wsaBuf.buf = (CHAR*)line;
				wsaBuf.len = sizeof(line);
			}

			DWORD flags = 0;
			std::byte line[1024] = {};
			WSABUF wsaBuf{};
			Weak<EventThread> thread;
		};

		struct WriteOp: Op
		{
			WriteOp(HANDLE handle_, Buffer&& buffer_, const Weak<EventThread>& thread_)
				: Op(handle_),
				  buffer(std::move(buffer_)),
				  thread(thread_)
			{
				wsaBuf.buf = (CHAR*)buffer.data();
				wsaBuf.len = (ULONG)buffer.count();
			}

			Buffer buffer;
			WSABUF wsaBuf{};
			Weak<EventThread> thread;
		};

		class OpSet
		{
			Mutex m_mutex;
			Map<Op*, Unique<Op>> m_ops;
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

				auto handle = (Op*) op.get();
				m_ops.insert(handle, std::move(op));
				return true;
			}

			Unique<Op> pop(Op* op)
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
			Map<EventThread*, Shared<EventThread>> m_threads;
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

		class SocketSource: public EventSource
		{
			WinOSEventLoop* m_eventLoop = nullptr;
			Unique<Socket> m_socket;
		public:
			SocketSource(Unique<Socket> socket, WinOSEventLoop* eventLoop)
				: m_socket(std::move(socket)),
				  m_eventLoop(eventLoop)
			{}

			HumanError accept(const Shared<EventThread>& thread) override
			{
				auto& allocator = m_eventLoop->m_allocator;
				auto& acceptEx = m_eventLoop->m_acceptEx;
				auto& ops = m_eventLoop->m_ops;

				auto socket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
				auto op = unique_from<AcceptOp>(allocator, std::move(socket), thread);
				auto acceptSocketHandle = (SOCKET)op->acceptSocket->fd();
				auto buffer = op->buffer;
				auto overlappedOp = (OVERLAPPED*)op.get();
				if (ops.tryPush(std::move(op)))
				{
					auto res = acceptEx(
						(SOCKET) m_socket->fd(),
						(SOCKET) acceptSocketHandle,
						buffer,
						0,
						sizeof(SOCKADDR_IN) + 16,
						sizeof(SOCKADDR_IN) + 16,
						NULL,
						overlappedOp
					);

					if (res == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
					{
						auto error = WSAGetLastError();
						return errf(allocator, "failed to schedule accept operation, ErrorCode({})"_sv, error);
					}
				}
				return {};
			}

			HumanError read(const Shared<EventThread>& thread) override
			{
				auto& allocator = m_eventLoop->m_allocator;
				auto& ops = m_eventLoop->m_ops;

				auto op = unique_from<ReadOp>(allocator, (HANDLE)m_socket->fd(), thread);
				auto wsaBuf = &op->wsaBuf;
				auto flags = &op->flags;
				auto overlappedOp = (OVERLAPPED*)op.get();
				if (ops.tryPush(std::move(op)))
				{
					auto res = WSARecv(
						(SOCKET)m_socket->fd(),
						wsaBuf,
						1,
						NULL,
						flags,
						overlappedOp,
						NULL
					);

					if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
					{
						auto error = WSAGetLastError();
						return errf(allocator, "failed to schedule read operation, ErrorCode({})"_sv, error);
					}
				}
				return {};
			}

			HumanError write(Span<const std::byte> bytes, const Shared<EventThread>& thread) override
			{
				auto& allocator = m_eventLoop->m_allocator;
				auto& ops = m_eventLoop->m_ops;

				Buffer buffer{allocator};
				buffer.push(bytes);
				auto op = unique_from<WriteOp>(allocator, (HANDLE)m_socket->fd(), std::move(buffer), thread);
				auto wsaBuf = &op->wsaBuf;
				auto overlappedOp = (OVERLAPPED*)op.get();
				if (ops.tryPush(std::move(op)))
				{
					auto res = WSASend(
						(SOCKET)m_socket->fd(),
						wsaBuf,
						1,
						NULL,
						0,
						overlappedOp,
						NULL
					);

					if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
					{
						auto error = WSAGetLastError();
						return errf(allocator, "failed to schedule write operation, ErrorCode({})"_sv, error);
					}
				}
				return {};
			}
		};

		void addThread(const Shared<EventThread>& thread) override
		{
			ZoneScoped;
			m_threads.push(thread);

			auto startEvent = unique_from<StartEvent2>(m_allocator);
			[[maybe_unused]] auto err = sendEventToThread(std::move(startEvent), thread);
			assert(!err);
		}

		Log* m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		LPFN_ACCEPTEX m_acceptEx = nullptr;
		OpSet m_ops;
		ThreadSet m_threads;
	public:
		WinOSEventLoop(HANDLE completionPort, LPFN_ACCEPTEX acceptEx, ThreadedEventLoop* parent, Log* log, Allocator* allocator)
			: EventLoop(parent, allocator),
			  m_log(log),
			  m_completionPort(completionPort),
			  m_acceptEx(acceptEx),
			  m_ops(allocator),
			  m_threads(allocator)
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
			m_ops.open();

			constexpr int MAX_ENTRIES = 128;
			OVERLAPPED_ENTRY entries[MAX_ENTRIES];
			while (true)
			{
				ULONG numEntries = 0;
				{
					ZoneScopedN("EventLoop2::GetQueuedCompletionStatusEx");
					auto res = GetQueuedCompletionStatusEx(m_completionPort, entries, MAX_ENTRIES, &numEntries, INFINITE, FALSE);
					if (res == FALSE)
					{
						auto error = GetLastError();
						return errf(m_allocator, "Failed to get queued completion status: ErrorCode({})"_sv, error);
					}
				}

				for (size_t i = 0; i < numEntries; ++i)
				{
					auto entry = entries[i];
					if (auto op = m_ops.pop((Op*)entry.lpOverlapped))
					{
						DWORD bytesTransferred = 0;
						if (op->handle != INVALID_HANDLE_VALUE)
							GetOverlappedResult(op->handle, entry.lpOverlapped, &bytesTransferred, FALSE);

						if (auto closeOp = dynamic_cast<CloseOp*>(op.get()))
						{
							m_ops.clear();
							m_threads.clear();
							return {};
						}
						else if (auto sendEventOp = dynamic_cast<SendEventOp*>(op.get()))
						{
							if (auto thread = sendEventOp->thread.lock())
							{
								if (auto err = thread->handle(sendEventOp->event.get()))
									return err;
							}
						}
						else if (auto stopThreadOp = dynamic_cast<StopThreadOp*>(op.get()))
						{
							if (auto thread = stopThreadOp->thread.lock())
								m_threads.pop(thread.get());
						}
						else if (auto acceptOp = dynamic_cast<AcceptOp*>(op.get()))
						{
							auto thread = acceptOp->thread.lock();
							if (thread)
							{
								if (thread->eventLoop() == this)
								{
									AcceptEvent2 acceptEvent{std::move(acceptOp->acceptSocket)};
									if (auto err = thread->handle(&acceptEvent))
										return err;
								}
								else
								{
									auto acceptEvent = unique_from<AcceptEvent2>(m_allocator, std::move(acceptOp->acceptSocket));
									if (auto err = thread->send(std::move(acceptEvent)))
										return err;
								}
							}
						}
						else if (auto readOp = dynamic_cast<ReadOp*>(op.get()))
						{
							auto thread = readOp->thread.lock();
							if (thread)
							{
								if (thread->eventLoop() == this)
								{
									ReadEvent readEvent{Span<const std::byte>{(const std::byte*)readOp->line, (size_t)bytesTransferred}, m_allocator};
									if (auto err = thread->handle(&readEvent))
										return err;
								}
								else
								{
									Buffer buffer{m_allocator};
									buffer.push(Span<const std::byte>{(const std::byte*)readOp->line, (size_t)bytesTransferred});
									auto readEvent = unique_from<ReadEvent>(m_allocator, std::move(buffer));
									if (auto err = thread->send(std::move(readEvent)))
										return err;
								}
							}
						}
						else if (auto writeOp = dynamic_cast<WriteOp*>(op.get()))
						{
							auto thread = writeOp->thread.lock();
							if (thread)
							{
								if (thread->eventLoop() == this)
								{
									WriteEvent writeEvent{bytesTransferred};
									if (auto err = thread->handle(&writeEvent))
										return err;
								}
								else
								{
									auto writeEvent = unique_from<WriteEvent>(m_allocator, bytesTransferred);
									if (auto err = thread->send(std::move(writeEvent)))
										return err;
								}
							}
						}
						else
						{
							assert(false);
							return errf(m_allocator, "unknown op type"_sv);
						}
					}
				}
			}
			return {};
		}

		void stop() override
		{
			auto op = unique_from<CloseOp>(m_allocator);
			auto handle = op.get();
			if (m_ops.tryPush(std::move(op)))
			{
				[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, (OVERLAPPED*)handle);
				assert(SUCCEEDED(res));
			}
		}

		void stopAllLoops() override
		{
			if (m_parentThreadedEventLoop)
			{
				m_parentThreadedEventLoop->stop();
			}
			else
			{
				stop();
			}
		}

		Result<EventSocket> registerSocket(Unique<Socket> socket) override
		{
			ZoneScoped;
			if (socket == nullptr)
				return nullptr;

			if (socket->type() == Socket::TYPE_TCP)
			{
				int flag = 1;
				auto ok = setsockopt(socket->fd(), IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag));
				if (ok == SOCKET_ERROR)
					return errf(m_allocator, "failed to set TCP_NODELAY flag, ErrorCode({})"_sv, WSAGetLastError());
			}

			auto newPort = CreateIoCompletionPort((HANDLE)socket->fd(), m_completionPort, NULL, 0);
			if (newPort != m_completionPort)
				return errf(m_allocator, "failed to register socket in completion port"_sv);

			auto res = shared_from<SocketSource>(m_allocator, std::move(socket), this);
			return EventSocket{res};
		}

		EventLoop* next() override
		{
			if (m_parentThreadedEventLoop)
				return m_parentThreadedEventLoop->next();
			return this;
		}

		HumanError sendEventToThread(Unique<Event> event, const Weak<EventThread>& thread)
		{
			auto op = unique_from<SendEventOp>(m_allocator, std::move(event), thread);
			auto handle = op.get();

			if (m_ops.tryPush(std::move(op)))
			{
				[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, (OVERLAPPED*)handle);
				assert(SUCCEEDED(res));
			}
			return {};
		}

		HumanError stopThread(const Weak<EventThread>& thread)
		{
			auto op = unique_from<StopThreadOp>(m_allocator, thread);
			auto handle = op.get();

			if (m_ops.tryPush(std::move(op)))
			{
				auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, (OVERLAPPED*)handle);
				if (FAILED(res))
					return errf(m_allocator, "failed to send stop thread event, ErrorCode({})"_sv, GetLastError());
			}
			return {};
		}
	};

	Result<Unique<EventLoop>> EventLoop::create(ThreadedEventLoop* parent, Log *log, Allocator *allocator)
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

		return unique_from<WinOSEventLoop>(allocator, completionPort, acceptEx, parent, log, allocator);
	}

	HumanError EventThread::send(Unique<Event> event)
	{
		ZoneScoped;
		auto linuxEventLoop = (WinOSEventLoop*)m_eventLoop;
		return linuxEventLoop->sendEventToThread(std::move(event), weakFromThis());
	}

	HumanError EventThread::stop()
	{
		ZoneScoped;
		auto linuxEventLoop = (WinOSEventLoop*)m_eventLoop;
		auto err = linuxEventLoop->stopThread(weakFromThis());
		if (!err)
			m_stopped.test_and_set();
		return err;
	}

	bool EventThread::stopped() const
	{
		return m_stopped.test();
	}
}