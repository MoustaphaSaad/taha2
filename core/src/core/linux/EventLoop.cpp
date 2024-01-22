#include "core/EventLoop.h"
#include "core/Hash.h"
#include "core/Queue.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>

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
			Queue<Op*> m_pollInOps;
		public:
			explicit LinuxEventSource(EventLoop* loop, Allocator* allocator)
				: EventSource(loop),
				  m_pollInOps(allocator)
			{}

			void pushPollIn(Op* op)
			{
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
			LinuxCloseEventSource(int eventfd, EventLoop* loop, Allocator* allocator)
				: LinuxEventSource(loop, allocator),
				  m_eventfd(eventfd)
			{}

			~LinuxCloseEventSource()
			{
				if (m_eventfd)
				{
					[[maybe_unused]] auto res = close(m_eventfd);
					assert(res == 0);
				}
			}

			void signalClose()
			{
				int64_t close = 1;
				[[maybe_unused]] auto res = ::write(m_eventfd, &close, sizeof(close));
				assert(res == sizeof(close));
			}
		};

		void pushPendingOp(Unique<Op> op, LinuxEventSource* source)
		{
			auto key = (Op*)op.get();

			switch (op->kind)
			{
			case Op::KIND_CLOSE:
				source->pushPollIn(key);
				break;
			default:
				assert(false);
				break;
			}

			m_scheduledOperations.insert(key, std::move(op));
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
		int m_epoll = 0;
		Shared<LinuxCloseEventSource> m_closeEventSource;
		Map<Op*, Unique<Op>> m_scheduledOperations;
	public:
		LinuxEventLoop(int epoll, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_scheduledOperations(allocator)
		{}

		~LinuxEventLoop() override
		{
			if (m_epoll)
			{
				[[maybe_unused]] auto res = close(m_epoll);
				assert(res == 0);
			}
		}

		HumanError run() override
		{
			int closeEvent = eventfd(0, 0);
			if (closeEvent == -1)
				return errf(m_allocator, "failed to create close event"_sv);

			m_closeEventSource = shared_from<LinuxCloseEventSource>(m_allocator, closeEvent, this, m_allocator);
			auto closeOp = unique_from<CloseOp>(m_allocator);
			pushPendingOp(std::move(closeOp), m_closeEventSource.get());

			epoll_event signal{};
			signal.events = EPOLLIN;
			signal.data.ptr = m_closeEventSource.get();
			int ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, closeEvent, &signal);
			if (ok == -1)
				return errf(m_allocator, "failed to add close event to epoll instance"_sv);

			constexpr int MAX_EVENTS = 32;
			epoll_event events[MAX_EVENTS];
			while (true)
			{
				auto count = epoll_wait(m_epoll, events, MAX_EVENTS, -1);
				if (count == -1)
				{
					if (errno == EINTR)
					{
						// that's an interrupt due to signal, just continue what we are doing
					}
					else
					{
						return errf(m_allocator, "epoll_wait failed, ErrorCode({})"_sv, errno);
					}
				}

				for (int i = 0; i < count; ++i)
				{
					auto event = events[i];
					auto source = (LinuxEventSource*) event.data.ptr;

					Op* opHandle = nullptr;
					if (event.events | EPOLLIN)
					{
						opHandle = source->popPollIn();
					}

					if (opHandle == nullptr)
						continue;


					auto op = popPendingOp((Op*)opHandle);
					assert(op != nullptr);

					switch (op->kind)
					{
					case Op::KIND_CLOSE:
						m_log->debug("event loop closed"_sv);
						return {};
					default:
						assert(false);
						return errf(m_allocator, "unknown event loop event kind, {}"_sv, (int)op->kind);
					}
				}
			}

			return {};
		}

		void stop() override
		{
			m_closeEventSource->signalClose();
		}

		Shared<EventSource> createEventSource(Unique<Socket> socket) override
		{
			// TODO: implement this function
			return nullptr;
		}

		HumanError read(EventSource* source, Reactor* reactor) override
		{
			// TODO: implement this function
			return {};
		}

		HumanError write(EventSource* source, Reactor* reactor, Span<const std::byte> buffer) override
		{
			// TODO: implement this function
			return {};
		}

		HumanError accept(EventSource* source, Reactor* reactor) override
		{
			// TODO: implement this function
			return {};
		}
	};

	Result<Unique<EventLoop>> EventLoop::create(Log* log, Allocator* allocator)
	{
		int epoll_fd = epoll_create1(0);
		if (epoll_fd == -1)
			return errf(allocator, "failed to create epoll"_sv);

		return unique_from<LinuxEventLoop>(allocator, epoll_fd, log, allocator);
	}
}