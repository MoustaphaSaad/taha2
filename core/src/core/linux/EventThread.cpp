#include "core/EventThread.h"
#include "core/Hash.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <tracy/Tracy.hpp>

namespace core
{
	class LinuxThreadPool: public EventThreadPool
	{
		struct Op
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

			explicit Op(KIND kind_)
				: kind(kind_)
			{}

			virtual ~Op() = default;

			KIND kind;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(KIND_CLOSE)
			{}
		};

		struct SendEventOp: Op
		{
			SendEventOp(int eventfd_, Unique<Event2> event_, const Shared<EventThread>& thread_)
				: Op(KIND_SEND_EVENT),
				  eventfd(eventfd_),
				  event(std::move(event_)),
				  thread(thread_)
			{}

			~SendEventOp() override
			{
				if (eventfd != -1)
				{
					[[maybe_unused]] auto res = ::close(eventfd);
					assert(res == 0);
				}
			}

			int eventfd = -1;
			Unique<Event2> event;
			Weak<EventThread> thread;
		};

		struct AcceptOp: Op
		{
			explicit AcceptOp(const Weak<EventThread>& thread_)
				: Op(KIND_ACCEPT),
				  thread(thread_)
			{}

			Weak<EventThread> thread;
		};

		struct ReadOp: Op
		{
			explicit ReadOp(const Weak<EventThread>& thread_)
				: Op(KIND_READ),
				  thread(thread_)
			{}

			Weak<EventThread> thread;
		};

		struct WriteOp: Op
		{
			WriteOp(Buffer&& buffer_, const Weak<EventThread>& thread_)
				: Op(KIND_WRITE),
				  thread(thread_),
				  buffer(std::move(buffer_)),
				  remainingBytes(buffer)
			{}

			Weak<EventThread> thread;
			Buffer buffer;
			Span<const std::byte> remainingBytes;
		};

		struct StopThreadOp: Op
		{
			StopThreadOp(int eventfd_, const Weak<EventThread> thread_)
				: Op(KIND_STOP_THREAD),
				  eventfd(eventfd_),
				  thread(thread_)
			{}

			~StopThreadOp() override
			{
				if (eventfd != -1)
				{
					[[maybe_unused]] auto res = ::close(eventfd);
					assert(res == 0);
				}
			}

			int eventfd = -1;
			Weak<EventThread> thread;
		};

		class OpQueue
		{
			Mutex m_mutex;
			Queue<Unique<Op>> m_ops;
		public:
			explicit OpQueue(Allocator* allocator)
				: m_mutex(allocator),
				  m_ops(allocator)
			{}

			void pushOpFront(Unique<Op> op)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				assert(op != nullptr);
				m_ops.push_front(std::move(op));
			}

			void pushOp(Unique<Op> op)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				assert(op != nullptr);
				m_ops.push_back(std::move(op));
			}

			Unique<Op> popOp()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);

				if (m_ops.count() > 0)
				{
					auto res = std::move(m_ops.front());
					m_ops.pop_front();
					return res;
				}
				return nullptr;
			}
		};

		class LinuxEventSource: public EventSource2
		{
			Allocator* m_allocator = nullptr;
			OpQueue m_pollInOps;
			OpQueue m_pollOutOps;
		public:
			explicit LinuxEventSource(Allocator* allocator)
				: m_allocator(allocator),
				  m_pollInOps(allocator),
				  m_pollOutOps(allocator)
			{}

			void pushPollIn(Unique<Op> op)
			{
				m_pollInOps.pushOp(std::move(op));
			}

			Unique<Op> popPollIn()
			{
				return m_pollInOps.popOp();
			}

			void pushPollOutFront(Unique<Op> op)
			{
				m_pollOutOps.pushOpFront(std::move(op));
			}

			void pushPollOut(Unique<Op> op)
			{
				m_pollOutOps.pushOp(std::move(op));
			}

			Unique<Op> popPollOut()
			{
				return m_pollOutOps.popOp();
			}

			HumanError accept(const Shared<EventThread>& thread) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}

			HumanError read(const Shared<EventThread>& thread) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}

			HumanError write(Span<const std::byte> buffer, const Shared<EventThread>& thread) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}

			virtual HumanError handlePollIn()
			{
				assert(false);
				return errf(m_allocator, "not implemented"_sv);
			}

			virtual HumanError handlePollOut()
			{
				assert(false);
				return errf(m_allocator, "not implemented"_sv);
			}

			virtual HumanError rearm()
			{
				assert(false);
				return errf(m_allocator, "not implemented"_sv);
			}
		};

		class LinuxCloseEventSource: public LinuxEventSource
		{
			int m_eventfd = -1;
		public:
			LinuxCloseEventSource(int eventfd, Allocator* allocator)
				: LinuxEventSource(allocator),
				  m_eventfd(eventfd)
			{}

			~LinuxCloseEventSource() override
			{
				if (m_eventfd != -1)
				{
					[[maybe_unused]] auto res = ::close(m_eventfd);
					assert(res == 0);
				}
			}

			void signalClose()
			{
				ZoneScoped;
				int64_t close = 1;
				[[maybe_unused]] auto res = ::write(m_eventfd, &close, sizeof(close));
				assert(res == sizeof(close));
			}
		};

		class LinuxEventSocket: public LinuxEventSource
		{
			LinuxThreadPool* m_eventThreadPool = nullptr;
			Unique<Socket> m_socket;
		public:
			LinuxEventSocket(Unique<Socket> socket, LinuxThreadPool* eventThreadPool)
				: LinuxEventSource(eventThreadPool->m_allocator),
				  m_eventThreadPool(eventThreadPool),
				  m_socket(std::move(socket))
			{}

			HumanError accept(const Shared<EventThread>& thread) override
			{
				auto& allocator = m_eventThreadPool->m_allocator;

				auto op = unique_from<AcceptOp>(allocator, thread);
				pushPollIn(std::move(op));
				return {};
			}

			HumanError read(const Shared<EventThread>& thread) override
			{
				auto& allocator = m_eventThreadPool->m_allocator;

				auto op = unique_from<ReadOp>(allocator, thread);
				pushPollIn(std::move(op));
				return {};
			}

			HumanError write(Span<const std::byte> bytes, const Shared<EventThread>& thread) override
			{
				auto& allocator = m_eventThreadPool->m_allocator;

				Buffer buffer{allocator};
				buffer.push(bytes);
				auto op = unique_from<WriteOp>(allocator, std::move(buffer), thread);
				pushPollOut(std::move(op));
				return {};
			}

			HumanError handlePollIn() override
			{
				auto& allocator = m_eventThreadPool->m_allocator;
				auto& threadPool = m_eventThreadPool->m_threadPool;

				auto op = popPollIn();
				if (op == nullptr)
					return {};

				switch (op->kind)
				{
				case Op::KIND_ACCEPT:
				{
					ZoneScopedN("accept");
					auto acceptOp = unique_static_cast<AcceptOp>(std::move(op));
					auto thread = acceptOp->thread.lock();
					auto acceptedSocket = m_socket->accept();
					if (threadPool)
					{
						auto func = Func<void()>{allocator, [acceptedSocket = std::move(acceptedSocket), thread]() mutable {
							AcceptEvent2 acceptEvent{std::move(acceptedSocket)};
							thread->handle(&acceptEvent);
						}};
						thread->executionQueue()->push(threadPool, std::move(func));
					}
					else
					{
						AcceptEvent2 acceptEvent{std::move(acceptedSocket)};
						thread->handle(&acceptEvent);
					}
					return {};
				}
				case Op::KIND_READ:
				{
					ZoneScopedN("read");
					auto readOp = unique_static_cast<ReadOp>(std::move(op));
					auto thread = readOp->thread.lock();
					std::byte line[1024];
					auto readSize = m_socket->read(line, sizeof(line));
					if (threadPool)
					{
						Buffer buffer{allocator};
						buffer.push(line, readSize);
						auto func = Func<void()>{allocator, [buffer = std::move(buffer), thread] {
							ReadEvent2 readEvent{Span<const std::byte>{buffer}};
							thread->handle(&readEvent);
						}};
						thread->executionQueue()->push(threadPool, std::move(func));
					}
					else
					{
						ReadEvent2 readEvent{Span<const std::byte>{line, readSize}};
						thread->handle(&readEvent);
					}
					return {};
				}
				default:
					assert(false);
					return errf(allocator, "unknown op"_sv);
				}
			}

			HumanError handlePollOut() override
			{
				auto& allocator = m_eventThreadPool->m_allocator;
				auto& threadPool = m_eventThreadPool->m_threadPool;

				auto op = popPollOut();
				if (op == nullptr)
					return {};

				switch (op->kind)
				{
				case Op::KIND_WRITE:
				{
					ZoneScopedN("write");
					auto writeOp = unique_static_cast<WriteOp>(std::move(op));
					auto thread = writeOp->thread.lock();
					auto writtenSize = m_socket->write(writeOp->remainingBytes.data(), writeOp->remainingBytes.count());
					if (writtenSize == writeOp->remainingBytes.count())
					{
						if (threadPool)
						{
							auto func = Func<void()>{allocator, [writtenSize, thread]
							{
								WriteEvent2 writeEvent{writtenSize};
								thread->handle(&writeEvent);
							}};
							thread->executionQueue()->push(threadPool, std::move(func));
						}
						else
						{
							WriteEvent2 writeEvent{writtenSize};
							thread->handle(&writeEvent);
						}
					}
					else
					{
						writeOp->remainingBytes = writeOp->remainingBytes.slice(writtenSize, writeOp->remainingBytes.count() - writtenSize);
						pushPollOutFront(std::move(writeOp));
					}
					return {};
				}
				default:
					assert(false);
					return errf(allocator, "unknown op"_sv);
				}
			}

			HumanError rearm() override
			{
				auto epoll = m_eventThreadPool->m_epoll;
				auto allocator = m_eventThreadPool->m_allocator;

				epoll_event sub{};
				sub.events = EPOLLIN | EPOLLOUT | EPOLLONESHOT;
				sub.data.ptr = this;
				auto ok = epoll_ctl(epoll, EPOLL_CTL_MOD, m_socket->fd(), &sub);
				if (ok == -1)
					return errf(allocator, "failed to rearm the socket source, ErrorCode({})"_sv, errno);
				return {};
			}
		};

		class SourceSet
		{
			Mutex m_mutex;
			Map<LinuxEventSource*, Weak<LinuxEventSource>> m_sources;
		public:
			explicit SourceSet(Allocator* allocator)
				: m_mutex(allocator),
				  m_sources(allocator)
			{}

			void pushSource(const Shared<LinuxEventSource>& source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				auto key = source.get();
				m_sources.insert(key, source);
			}

			void removeSource(LinuxEventSource* source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_sources.remove(source);
			}

			Shared<LinuxEventSource> popSource(LinuxEventSource* source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				auto it = m_sources.lookup(source);
				if (it == m_sources.end())
					return nullptr;
				return it->value.lock();
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

		Log* m_log = nullptr;
		int m_epoll = -1;
		Shared<LinuxCloseEventSource> m_closeEventSource;
		SourceSet m_sources;
		ThreadSet m_threads;
		OpSet m_ops;
		ThreadPool* m_threadPool = nullptr;

		void addThread(const Shared<EventThread>& thread) override
		{
			ZoneScoped;
			m_threads.push(thread);

			auto startEvent = unique_from<StartEvent>(m_allocator);
			[[maybe_unused]] auto err = sendEvent(std::move(startEvent), thread);
			assert(!err);
		}

	protected:
		HumanError sendEvent(Unique<Event2> event, const Shared<EventThread>& thread) override
		{
			ZoneScoped;
			auto fd = eventfd(0, 0);
			if (fd == -1)
				return errf(m_allocator, "failed to create eventfd, ErrorCode({})"_sv, errno);
			auto op = unique_from<SendEventOp>(m_allocator, fd, std::move(event), thread);

			epoll_event closeEvent{};
			closeEvent.events = EPOLLIN | EPOLLONESHOT;
			closeEvent.data.ptr = op.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &closeEvent);
			if (ok == -1)
				return errf(m_allocator, "failed to add close event to epoll, ErrorCode({})"_sv, errno);

			if (m_ops.tryPush(std::move(op)))
			{
				int64_t v = 1;
				[[maybe_unused]] auto res = ::write(fd, &v, sizeof(v));
				assert(res == sizeof(v));
			}
			return {};
		}

	public:
		LinuxThreadPool(ThreadPool* threadPool, int epoll, Log* log, Allocator* allocator)
			: EventThreadPool(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_sources(allocator),
			  m_threads(allocator),
			  m_ops(allocator),
			  m_threadPool(threadPool)
		{}

		~LinuxThreadPool() override
		{
			if (m_epoll == -1)
			{
				[[maybe_unused]] auto res = ::close(m_epoll);
				assert(res == 0);
				m_epoll = -1;
			}
		}

		HumanError run() override
		{
			auto closefd = eventfd(0, 0);
			if (closefd == -1)
				return errf(m_allocator, "failed to create close event, ErrorCode({})"_sv, errno);
			m_closeEventSource = shared_from<LinuxCloseEventSource>(m_allocator, closefd, m_allocator);
			m_sources.pushSource(m_closeEventSource);
			m_closeEventSource->pushPollIn(unique_from<CloseOp>(m_allocator));

			epoll_event closeEvent{};
			closeEvent.events = EPOLLIN | EPOLLONESHOT;
			closeEvent.data.ptr = m_closeEventSource.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, closefd, &closeEvent);
			if (ok == -1)
				return errf(m_allocator, "failed to add close event to epoll, ErrorCode({})"_sv, errno);

			constexpr int MAX_EVENTS = 128;
			epoll_event events[MAX_EVENTS];
			while (true)
			{
				int numEntries = 0;
				{
					ZoneScopedN("EventThreadPool::epoll_wait");
					numEntries = epoll_wait(m_epoll, events, MAX_EVENTS, -1);
					if (numEntries == -1)
					{
						if (errno == EINTR)
						{
							// try again, this can be due to signal handling, like debugger
							continue;
						}
						return errf(m_allocator, "epoll_wait failed, ErrorCode({})"_sv, errno);
					}
				}

				for (int i = 0; i < numEntries; ++i)
				{
					auto event = events[i];
					if (auto source = m_sources.popSource((LinuxEventSource*)event.data.ptr))
					{
						if (event.events & EPOLLIN)
						{
							// close event
							if (source.get() == (LinuxEventSource *) m_closeEventSource.get())
							{
								m_log->debug("event loop closed"_sv);
								m_closeEventSource = nullptr;
								return {};
							}
							else
							{
								if (auto err = source->handlePollIn())
									return err;
							}
						}
						if (event.events & EPOLLOUT)
						{
							if (auto err = source->handlePollOut())
								return err;
						}
						if (auto err = source->rearm())
							return err;
					}
					else if (auto op = m_ops.pop((Op*)event.data.ptr))
					{
						switch (op->kind)
						{
						case Op::KIND_SEND_EVENT:
						{
							auto sendEventOp = unique_static_cast<SendEventOp>(std::move(op));
							auto thread = sendEventOp->thread.lock();
							if (m_threadPool)
							{
								auto func = Func<void()>{m_allocator, [event = std::move(sendEventOp->event), thread]{
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
						case Op::KIND_STOP_THREAD:
						{
							auto stopThreadOp = unique_static_cast<StopThreadOp>(std::move(op));
							if (auto thread = stopThreadOp->thread.lock())
								m_threads.pop(thread.get());
							break;
						}
						default:
							assert(false);
							break;
						}
					}
					else
					{
						assert(false);
					}
				}
			}
			return {};
		}

		void stop() override
		{
			ZoneScoped;
			m_log->debug("stop"_sv);
			m_closeEventSource->signalClose();
		}

		Result<EventSocket> registerSocket(Unique<Socket> socket) override
		{
			auto fd = socket->fd();
			auto res = shared_from<LinuxEventSocket>(m_allocator, std::move(socket), this);
			epoll_event sub{};
			sub.events = EPOLLIN | EPOLLOUT | EPOLLONESHOT;
			sub.data.ptr = res.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &sub);
			if (ok == -1)
				return errf(m_allocator, "failed to register socket, ErrorCode({})"_sv, errno);

			m_sources.pushSource(res);
			return EventSocket{res};
		}

		void stopThread(const Shared<EventThread>& thread) override
		{
			auto fd = eventfd(0, 0);
			assert(fd != -1);
			auto op = unique_from<StopThreadOp>(m_allocator, fd, thread);
			if (m_ops.tryPush(std::move(op)))
			{
				int64_t v = 1;
				[[maybe_unused]] auto res = ::write(fd, &v, sizeof(v));
				assert(res == sizeof(v));
			}
		}
	};

	Result<Unique<EventThreadPool>> EventThreadPool::create(ThreadPool *threadPool, Log *log, Allocator *allocator)
	{
		auto epoll = epoll_create1(0);
		if (epoll == -1)
			return errf(allocator, "failed to create epoll, ErrorCode({})"_sv, errno);

		return unique_from<LinuxThreadPool>(allocator, threadPool, epoll, log, allocator);
	}
}