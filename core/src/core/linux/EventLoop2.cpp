#include "core/EventLoop2.h"
#include "core/Mutex.h"
#include "core/Hash.h"
#include "core/Queue.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include <tracy/Tracy.hpp>

namespace core
{
	class LinuxEventLoop2: public EventLoop2
	{
		struct Op
		{
			virtual ~Op() = default;
		};

		struct CloseOp: Op
		{
			CloseOp(int eventfd_)
				: eventfd(eventfd_)
			{}

			~CloseOp() override
			{
				if (eventfd != -1)
				{
					[[maybe_unused]] auto res = ::close(eventfd);
					assert(res == 0);
				}
			}

			int eventfd = -1;
		};

		struct SendEventOp: Op
		{
			SendEventOp(int eventfd_, Unique<Event2> event_, const Weak<EventThread2>& thread_)
				: eventfd(eventfd_),
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
			Weak<EventThread2> thread;
		};

		struct StopThreadOp: Op
		{
			StopThreadOp(int eventfd_, const Weak<EventThread2>& thread_)
				: eventfd(eventfd_),
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
			Weak<EventThread2> thread;
		};

		struct AcceptOp: Op
		{
			explicit AcceptOp(const Weak<EventThread2>& thread_)
				: thread(thread_)
			{}

			Weak<EventThread2> thread;
		};

		struct ReadOp: Op
		{
			explicit ReadOp(const Weak<EventThread2>& thread_)
				: thread(thread_)
			{}

			Weak<EventThread2> thread;
		};

		struct WriteOp: Op
		{
			WriteOp(Buffer&& buffer_, const Weak<EventThread2>& thread_)
				: buffer(std::move(buffer_)),
				  thread(thread_),
				  remainingBytes(buffer)
			{}

			Buffer buffer;
			Weak<EventThread2> thread;
			Span<const std::byte> remainingBytes;
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
			Map<EventThread2*, Shared<EventThread2>> m_threads;
		public:
			explicit ThreadSet(Allocator* allocator)
				: m_mutex(allocator),
				  m_threads(allocator)
			{}

			void push(const Shared<EventThread2>& thread)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				auto handle = thread.get();
				m_threads.insert(handle, thread);
			}

			void pop(EventThread2* handle)
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

		class SourceSet
		{
			Mutex m_mutex;
			Map<EventSource2*, Weak<EventSource2>> m_sources;
		public:
			explicit SourceSet(Allocator* allocator)
				: m_mutex(allocator),
				  m_sources(allocator)
			{}

			void push(const Shared<EventSource2>& source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				auto key = source.get();
				m_sources.insert(key, source);
			}

			void remove(EventSource2* source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_sources.remove(source);
			}

			Shared<EventSource2> pop(EventSource2* source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				auto it = m_sources.lookup(source);
				if (it == m_sources.end())
					return nullptr;
				return it->value.lock();
			}
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

			void push(Unique<Op> op)
			{
				assert(op != nullptr);
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_ops.push_back(std::move(op));
			}

			Unique<Op> pop()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				if (m_ops.count() == 0)
					return nullptr;

				auto res = std::move(m_ops.front());
				m_ops.pop_front();
				return res;
			}

			Op* peek()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				if (m_ops.count() == 0)
					return nullptr;
				return m_ops.front().get();
			}
		};

		class SocketSource: public EventSource2
		{
			LinuxEventLoop2* m_eventLoop = nullptr;
			Unique<Socket> m_socket;
			OpQueue m_pollIn;
			OpQueue m_pollOut;
		public:
			SocketSource(Unique<Socket> socket, LinuxEventLoop2* eventLoop)
				: m_socket(std::move(socket)),
				  m_eventLoop(eventLoop),
				  m_pollIn(eventLoop->m_allocator),
				  m_pollOut(eventLoop->m_allocator)
			{}

			HumanError accept(const Shared<EventThread2>& thread) override
			{
				auto allocator = m_eventLoop->m_allocator;

				auto op = unique_from<AcceptOp>(allocator, thread);
				m_pollIn.push(std::move(op));
				return {};
			}

			HumanError read(const Shared<EventThread2>& thread) override
			{
				auto allocator = m_eventLoop->m_allocator;

				auto op = unique_from<ReadOp>(allocator, thread);
				m_pollIn.push(std::move(op));
				return {};
			}

			HumanError write(Span<const std::byte> bytes, const Shared<EventThread2>& thread) override
			{
				auto allocator = m_eventLoop->m_allocator;

				Buffer buffer{allocator};
				buffer.push(bytes);
				auto op = unique_from<WriteOp>(allocator, std::move(buffer), thread);
				m_pollOut.push(std::move(op));
				return {};
			}

			HumanError handlePollIn()
			{
				auto& allocator = m_eventLoop->m_allocator;
				std::byte line[2048] = {};

				while (auto op = m_pollIn.peek())
				{
					if (auto acceptOp = dynamic_cast<AcceptOp*>(op))
					{
						auto thread = acceptOp->thread.lock();
						auto acceptedSocket = m_socket->accept();
						if (acceptedSocket == nullptr)
						{
							if (errno == EAGAIN || errno == EWOULDBLOCK)
								return {};
							else
								return errf(allocator, "failed to accept connection, ErrorCode({})"_sv, errno);
						}
						m_pollIn.pop();
						if (thread)
						{
							if (m_eventLoop == thread->eventLoop())
							{
								AcceptEvent2 acceptEvent{std::move(acceptedSocket)};
								if (auto err = thread->handle(&acceptEvent))
									return err;
							}
							else
							{
								auto acceptEvent = unique_from<AcceptEvent2>(allocator, std::move(acceptedSocket));
								if (auto err = thread->send(std::move(acceptEvent)))
									return err;
							}
						}
					}
					else if (auto readOp = dynamic_cast<ReadOp*>(op))
					{
						auto thread = readOp->thread.lock();
						auto readBytes = ::read((int)m_socket->fd(), line, sizeof(line));
						if (readBytes == -1)
						{
							if (errno = EAGAIN || errno == EWOULDBLOCK)
								return {};
							else
								return errf(allocator, "failed to read from socket, ErrorCode({})"_sv, errno);
						}
						m_pollIn.pop();
						if (thread)
						{
							if (m_eventLoop == thread->eventLoop())
							{
								ReadEvent2 readEvent{Span<const std::byte>{line, (size_t)readBytes}, allocator};
								if (auto err = thread->handle(&readEvent))
									return err;
							}
							else
							{
								Buffer buffer{allocator};
								buffer.push(Span<const std::byte>{line, (size_t)readBytes});
								auto readEvent = unique_from<ReadEvent2>(allocator, std::move(buffer));
								if (auto err = thread->send(std::move(readEvent)))
									return err;
							}
						}
					}
					break;
				}

				return {};
			}

			HumanError handlePollOut()
			{
				auto& allocator = m_eventLoop->m_allocator;

				while (auto op = m_pollOut.peek())
				{
					if (auto writeOp = dynamic_cast<WriteOp*>(op))
					{
						auto thread = writeOp->thread.lock();
						auto writtenBytes = ::write((int)m_socket->fd(), writeOp->remainingBytes.data(), writeOp->remainingBytes.sizeInBytes());
						if (writtenBytes == -1)
						{
							if (errno == EAGAIN || errno == EWOULDBLOCK)
								return {};
							else
								return errf(allocator, "failed to write to socket, ErrorCode({})"_sv, errno);
						}
						writeOp->remainingBytes = writeOp->remainingBytes.slice(writtenBytes, writeOp->remainingBytes.count() - writtenBytes);

						if (writeOp->remainingBytes.count() == 0)
						{
							m_pollOut.pop();
							if (thread)
							{
								if (m_eventLoop == thread->eventLoop())
								{
									WriteEvent2 writeEvent{writeOp->buffer.count()};
									if (auto err = thread->handle(&writeEvent))
										return err;
								}
								else
								{
									auto writeEvent = unique_from<WriteEvent2>(allocator, writeOp->buffer.count());
									if (auto err = thread->send(std::move(writeEvent)))
										return err;
								}
							}
						}
					}
					break;
				}

				return {};
			}
		};

		void addThread(const Shared<EventThread2>& thread) override
		{
			ZoneScoped;
			m_threads.push(thread);

			auto startEvent = unique_from<StartEvent2>(m_allocator);
			[[maybe_unused]] auto err = sendEventToThread(std::move(startEvent), thread);
			assert(!err);
		}

		Log* m_log = nullptr;
		int m_epoll = -1;
		OpSet m_ops;
		ThreadSet m_threads;
		SourceSet m_sources;
	public:
		LinuxEventLoop2(int epoll, Log* log, Allocator* allocator)
			: EventLoop2(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_ops(allocator),
			  m_threads(allocator),
			  m_sources(allocator)
		{}

		~LinuxEventLoop2() override
		{
			if (m_epoll != -1)
			{
				[[maybe_unused]] auto res = ::close(m_epoll);
				assert(res == 0);
				m_epoll = -1;
			}
		}

		HumanError run() override
		{
			m_ops.open();

			constexpr int MAX_EVENTS = 128;
			epoll_event events[MAX_EVENTS];
			while (true)
			{
				int numEntries = 0;
				{
					ZoneScopedN("EventLoop2::epoll_wait");
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
					if (auto op = m_ops.pop((Op*)event.data.ptr))
					{
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
					}
					else if (auto source = m_sources.pop((EventSource2*)event.data.ptr))
					{
						if (auto socketSource = dynamic_cast<SocketSource*>(source.get()))
						{
							if (event.events & EPOLLIN)
							{
								if (auto err = socketSource->handlePollIn())
									return err;
							}
							if (event.events & EPOLLOUT)
							{
								if (auto err = socketSource->handlePollOut())
									return err;
							}
						}
					}
					else
					{
						assert(false);
					}
				}
			}
		}

		void stop() override
		{
			ZoneScoped;

			auto fd = eventfd(0, 0);
			assert(fd != -1);
			auto op = unique_from<CloseOp>(m_allocator, fd);
			auto handle = op.get();

			if (m_ops.tryPush(std::move(op)))
			{
				epoll_event sub{};
				sub.events = EPOLLIN | EPOLLONESHOT;
				sub.data.ptr = handle;
				[[maybe_unused]] auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &sub);
				assert(ok != -1);

				int64_t v = 1;
				[[maybe_unused]] auto res = ::write(fd, &v, sizeof(v));
				assert(res == sizeof(v));
			}

			m_ops.close();
		}

		Result<EventSocket2> registerSocket(Unique<Socket> socket) override
		{
			auto fd = socket->fd();

			auto flags = fcntl(fd, F_GETFL, 0);
			if (flags == -1)
				return errf(m_allocator, "failed to get socket flags, ErrorCode({})"_sv, errno);
			flags |= O_NONBLOCK;
			if (fcntl(fd, F_SETFL, flags) != 0)
				return errf(m_allocator, "failed to make socket non-blocking, ErrorCode({})"_sv, errno);

			int yes = 1;
			if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) != 0)
				return errf(m_allocator, "failed to set TCP socket no delay, ErrorCode({})"_sv, errno);

			auto res = shared_from<SocketSource>(m_allocator, std::move(socket), this);

			epoll_event sub{};
			sub.events = EPOLLIN | EPOLLOUT;
			sub.data.ptr = res.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &sub);
			if (ok == -1)
				return errf(m_allocator, "failed to register socket, ErrorCode({})"_sv, errno);

			m_sources.push(res);
			return EventSocket2{res};
		}

		HumanError sendEventToThread(Unique<Event2> event, const Weak<EventThread2>& thread)
		{
			auto fd = eventfd(0, 0);
			if (fd == -1)
				return errf(m_allocator, "failed to create eventfd, ErrorCode({})"_sv, errno);
			auto op = unique_from<SendEventOp>(m_allocator, fd, std::move(event), thread);

			epoll_event sub{};
			sub.events = EPOLLIN | EPOLLONESHOT;
			sub.data.ptr = op.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &sub);
			if (ok == -1)
				return errf(m_allocator, "failed to register event in epoll, ErrorCode({})"_sv, errno);

			if (m_ops.tryPush(std::move(op)))
			{
				int64_t v = 1;
				auto res = ::write(fd, &v, sizeof(v));
				if (res != sizeof(v))
					return errf(m_allocator, "failed to signal eventfd, ErrorCode({})"_sv, errno);
			}
			return {};
		}

		void stopThread(const Weak<EventThread2>& thread)
		{
			auto fd = eventfd(0, 0);
			assert(fd != -1);
			auto op = unique_from<StopThreadOp>(m_allocator, fd, thread);

			epoll_event sub{};
			sub.events = EPOLLIN | EPOLLONESHOT;
			sub.data.ptr = op.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &sub);
			assert(ok != -1);

			if (m_ops.tryPush(std::move(op)))
			{
				int64_t v = 1;
				auto res = ::write(fd, &v, sizeof(v));
				assert(res == sizeof(v));
			}
		}
	};

	Result<Unique<EventLoop2>> EventLoop2::create(Log *log, Allocator *allocator)
	{
		auto epoll = epoll_create1(0);
		if (epoll == -1)
			return errf(allocator, "failed to create epoll, ErrorCode({})"_sv, errno);

		return unique_from<LinuxEventLoop2>(allocator, epoll, log, allocator);
	}

	HumanError EventThread2::send(Unique<Event2> event)
	{
		ZoneScoped;
		auto linuxEventLoop = (LinuxEventLoop2*)m_eventLoop;
		return linuxEventLoop->sendEventToThread(std::move(event), weakFromThis());
	}

	void EventThread2::stop()
	{
		ZoneScoped;
		auto linuxEventLoop = (LinuxEventLoop2*)m_eventLoop;
		return linuxEventLoop->stopThread(weakFromThis());
	}
}