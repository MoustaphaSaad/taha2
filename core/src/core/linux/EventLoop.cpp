#include "core/EventLoop.h"
#include "core/Hash.h"
#include "core/Queue.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <tracy/Tracy.hpp>

namespace core
{
	class LinuxEventLoop: public EventLoop
	{
		struct Op
		{
			enum KIND
			{
				KIND_NONE,
				KIND_CLOSE,
			};

			explicit Op(KIND kind_)
				: kind(kind_)
			{}

			KIND kind;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(KIND_CLOSE)
			{}
		};

		class LinuxEventSource: public EventSource
		{
			Allocator* m_allocator = nullptr;
			LinuxEventLoop* m_loop = nullptr;
			Queue<Op*> m_pollInOps;
		public:
			explicit LinuxEventSource(LinuxEventLoop* loop, Allocator* allocator)
				: EventSource(loop),
				  m_allocator(allocator),
				  m_loop(loop),
				  m_pollInOps(allocator)
			{}

			~LinuxEventSource() override
			{
				m_loop->removeSource(this);
			}

			void pushPollIn(Op* op)
			{
				assert(op != nullptr);
				m_pollInOps.push_back(op);
			}

			Op* popPollIn()
			{
				if (m_pollInOps.count() > 0)
				{
					auto res = m_pollInOps.front();
					m_pollInOps.pop_front();
					return res;
				}
				return nullptr;
			}
		};

		class LinuxCloseEventSource: public LinuxEventSource
		{
			int m_eventfd = 0;
		public:
			LinuxCloseEventSource(int eventfd, LinuxEventLoop* loop, Allocator* allocator)
				: LinuxEventSource(loop, allocator),
				  m_eventfd(eventfd)
			{}

			~LinuxCloseEventSource() override
			{
				if (m_eventfd)
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

		void pushSource(const Shared<LinuxEventSource>& source)
		{
			auto key = source.get();
			m_log->debug("push source: {}"_sv, (void*)source.get());
			m_aliveSources.insert(key, source);
		}

		void removeSource(LinuxEventSource* source)
		{
			m_log->debug("remove source: {}"_sv, (void*)source);
			[[maybe_unused]] auto res = m_aliveSources.remove(source);
			assert(res == true);
		}

		void pushPendingOp(Unique<Op> op, LinuxEventSource* source)
		{
			assert(op != nullptr);
			assert(source != nullptr && m_aliveSources.lookup(source) != m_aliveSources.end());
			auto key = op.get();

			switch (op->kind)
			{
			case Op::KIND_CLOSE:
				source->pushPollIn(key);
				break;
			default:
				assert(false);
				break;
			}

			m_pendingOps.insert(key, std::move(op));
		}

		Shared<LinuxEventSource> resolveSource(LinuxEventSource* source)
		{
			auto it = m_aliveSources.lookup(source);
			if (it == m_aliveSources.end())
				return nullptr;
			auto& weakHandle = it->value;
			if (weakHandle.expired())
				return nullptr;
			return weakHandle.lock();
		}

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		int m_epoll = 0;
		Shared<LinuxCloseEventSource> m_closeEventSource;
		Map<LinuxEventSource*, Weak<LinuxEventSource>> m_aliveSources;
		Map<Op*, Unique<Op>> m_pendingOps;
	public:
		LinuxEventLoop(int epoll, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_aliveSources(allocator),
			  m_pendingOps(allocator)
		{}

		~LinuxEventLoop() override
		{
			ZoneScoped;
			if (m_epoll)
			{
				[[maybe_unused]] auto res = ::close(m_epoll);
				assert(res == 0);
			}
		}

		HumanError run() override
		{
			ZoneScoped;

			auto closefd = eventfd(0, 0);
			if (closefd == -1)
				return errf(m_allocator, "Failed to create close event, ErrorCode({})"_sv, errno);
			m_closeEventSource = shared_from<LinuxCloseEventSource>(m_allocator, closefd, this, m_allocator);
			pushSource(m_closeEventSource);
			pushPendingOp(unique_from<CloseOp>(m_allocator), m_closeEventSource.get());

			epoll_event closeSubscription{};
			closeSubscription.events = EPOLLIN | EPOLLET;
			closeSubscription.data.ptr = m_closeEventSource.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, closefd, &closeSubscription);
			if (ok == -1)
				return errf(m_allocator, "failed to add close event to epoll instance, ErrorCode({})"_sv, errno);

			constexpr int MAX_EVENTS = 32;
			epoll_event events[MAX_EVENTS];
			while (true)
			{
				int count = 0;

				{
					ZoneScopedN("epoll_wait");
					count = epoll_wait(m_epoll, events, MAX_EVENTS, -1);
					if (count == -1)
					{
						if (errno == EINTR)
						{
							// try again, this can be due to signal handling, like debugger
							continue;
						}
						return errf(m_allocator, "epoll_wait failed, ErrorCode({})"_sv, errno);
					}
				}

				for (int i = 0; i < count; ++i)
				{
					ZoneScopedN("handleEvent");

					auto event = events[i];
					auto sourceHandle = (LinuxEventSource*)event.data.ptr;
					auto source = resolveSource(sourceHandle);
					assert(source != nullptr);

					if (event.events | EPOLLIN)
					{
						// close event
						if (source.get() == (LinuxEventSource*)m_closeEventSource.get())
						{
							m_log->debug("event loop closed"_sv);
							m_closeEventSource = nullptr;
							m_pendingOps.clear();
							return {};
						}
					}
				}
			}

			return errf(m_allocator, "not implemented"_sv);
		}
		void stop() override
		{
			ZoneScoped;
			m_log->debug("stop"_sv);
			m_closeEventSource->signalClose();
		}

		Shared<EventSource> createEventSource(Unique<Socket> socket) override
		{
			return nullptr;
		}
		HumanError read(EventSource* source, Reactor* reactor) override
		{
			return errf(m_allocator, "not implemented"_sv);
		}
		HumanError write(EventSource* source, Reactor* reactor, Span<const std::byte> buffer) override
		{
			return errf(m_allocator, "not implemented"_sv);
		}
		HumanError accept(EventSource* source, Reactor* reactor) override
		{
			return errf(m_allocator, "not implemented"_sv);
		}
	};

	Result<Unique<EventLoop>> EventLoop::create(Log *log, Allocator *allocator)
	{
		ZoneScoped;
		auto epoll_fd = epoll_create1(0);
		if (epoll_fd == -1)
			return errf(allocator, "Failed to create epoll, ErrorCode({})"_sv, errno);

		return unique_from<LinuxEventLoop>(allocator, epoll_fd, log, allocator);
	}
}