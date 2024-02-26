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

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		int m_epoll = -1;
		OpSet m_ops;
	public:
		LinuxEventLoop2(int epoll, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_ops(allocator)
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
							return {};
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
			m_log->debug("stop"_sv);

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
	};

	Result<Unique<EventLoop2>> EventLoop2::create(Log *log, Allocator *allocator)
	{
		auto epoll = epoll_create1(0);
		if (epoll == -1)
			return errf(allocator, "failed to create epoll, ErrorCode({})"_sv, errno);

		return unique_from<LinuxEventLoop2>(allocator, epoll, log, allocator);
	}
}