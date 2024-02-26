#include "core/EventLoop2.h"
#include "core/Mutex.h"
#include "core/Hash.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>

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
	public:
		LinuxEventLoop2(int epoll, Log* log, Allocator* allocator)
			: EventLoop2(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_ops(allocator),
			  m_threads(allocator)
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