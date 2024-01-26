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
				KIND_ACCEPT,
				KIND_READ,
				KIND_WRITE,
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

		struct AcceptOp: Op
		{
			Reactor* reactor = nullptr;

			AcceptOp(Reactor* reactor_)
				: Op(KIND_ACCEPT),
				  reactor(reactor_)
			{}
		};

		struct ReadOp: Op
		{
			Reactor* reactor = nullptr;

			ReadOp(Reactor* reactor_)
				: Op(KIND_READ),
				  reactor(reactor_)
			{}
		};

		struct WriteOp: Op
		{
			Reactor* reactor = nullptr;
			Buffer buffer;
			Span<const std::byte> remainingBytes;

			WriteOp(Reactor* reactor_, Buffer&& buffer_)
				: Op(KIND_WRITE),
				  reactor(reactor_),
				  buffer(std::move(buffer_)),
				  remainingBytes(buffer)
			{}
		};

		class LinuxEventSource: public EventSource
		{
			Allocator* m_allocator = nullptr;
			Queue<Op*> m_pollInOps;
			Queue<Op*> m_pollOutOps;
		public:
			explicit LinuxEventSource(LinuxEventLoop* loop, Allocator* allocator)
				: EventSource(loop),
				  m_allocator(allocator),
				  m_pollInOps(allocator),
				  m_pollOutOps(allocator)
			{}

			~LinuxEventSource() override
			{
				auto loop = (LinuxEventLoop*)eventLoop();
				loop->removeSource(this);
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

			Op* peekPollIn()
			{
				if (m_pollInOps.count() > 0)
					return m_pollInOps.front();
				return nullptr;
			}

			void pushPollOut(Op* op)
			{
				assert(op != nullptr);
				m_pollOutOps.push_back(op);
			}

			Op* popPollOut()
			{
				if (m_pollOutOps.count() > 0)
				{
					auto res = m_pollOutOps.front();
					m_pollOutOps.pop_front();
					return res;
				}
				return nullptr;
			}

			Op* peekPollOut()
			{
				if (m_pollOutOps.count() > 0)
					return m_pollOutOps.front();
				return nullptr;
			}

			Allocator* allocator() const { return m_allocator; }

			virtual HumanError handlePollIn()
			{
				return errf(m_allocator, "not implemented"_sv);
			}

			virtual HumanError handlePollOut()
			{
				return errf(m_allocator, "not implemented"_sv);
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

		class LinuxSocketEventSource: public LinuxEventSource
		{
			Unique<Socket> m_socket;
		public:
			LinuxSocketEventSource(Unique<Socket> socket, LinuxEventLoop* loop, Allocator* allocator)
				: LinuxEventSource(loop, allocator),
				  m_socket(std::move(socket))
			{}

			HumanError handlePollIn() override
			{
				ZoneScoped;
				auto loop = (LinuxEventLoop*)eventLoop();

				static std::byte line[1024] = {};
				while (auto peekOp = peekPollIn())
				{
					switch (peekOp->kind)
					{
					case Op::KIND_ACCEPT:
					{
						ZoneScopedN("accept");
						auto acceptedSocket = m_socket->accept();
						if (acceptedSocket == nullptr)
						{
							if (errno == EAGAIN || errno == EWOULDBLOCK)
								return {};
							else
								return errf(allocator(), "failed to accept socket, ErrorCode({})"_sv, errno);
						}
						auto opHandle = popPollIn();
						auto op = loop->popPendingOp(opHandle);
						assert(op != nullptr);
						auto acceptOp = unique_static_cast<AcceptOp>(std::move(op));
						if (acceptOp->reactor)
						{
							AcceptEvent acceptEvent{std::move(acceptedSocket), this};
							acceptOp->reactor->handle(&acceptEvent);
						}
						break;
					}
					case Op::KIND_READ:
					{
						ZoneScopedN("read");
						auto readBytes = ::read((int)m_socket->fd(), line, sizeof(line));
						if (readBytes == -1)
						{
							if (errno == EAGAIN || errno == EWOULDBLOCK)
								return {};
							else
								return errf(allocator(), "failed to read from socket, ErrorCode({})"_sv, errno);
						}
						auto opHandle = popPollIn();
						auto op = loop->popPendingOp(opHandle);
						assert(op != nullptr);
						auto readOp = unique_static_cast<ReadOp>(std::move(op));
						if (readOp->reactor)
						{
							ReadEvent readEvent{Span<const std::byte>{line, (size_t)readBytes}, this};
							readOp->reactor->handle(&readEvent);
						}
						break;
					}
					default:
						assert(false);
						break;
					}
				}
				return {};
			}

			HumanError handlePollOut() override
			{
				ZoneScoped;
				auto loop = (LinuxEventLoop*)eventLoop();

				while (auto peekOp = peekPollOut())
				{
					switch (peekOp->kind)
					{
					case Op::KIND_WRITE:
					{
						ZoneScopedN("write");
						auto op = (WriteOp*)peekOp;
						auto writtenBytes = ::write((int)m_socket->fd(), op->remainingBytes.data(), op->remainingBytes.count());
						if (writtenBytes == -1)
						{
							if (errno == EAGAIN || errno == EWOULDBLOCK)
								return {};
							else
								return errf(allocator(), "failed to write to socket, ErrorCode({})"_sv, errno);
						}
						auto opHandle = popPollOut();
						auto writeOpHandle = loop->popPendingOp(opHandle);
						assert(writeOpHandle != nullptr);
						auto writeOp = unique_static_cast<WriteOp>(std::move(writeOpHandle));
						if (writeOp->reactor)
						{
							WriteEvent writeEvent{(size_t)writtenBytes, this};
							writeOp->reactor->handle(&writeEvent);
						}
						break;
					}
					default:
						assert(false);
						break;
					}
				}
				return {};
			}
		};

		void pushSource(const Shared<LinuxEventSource>& source)
		{
			auto key = source.get();
			m_aliveSources.insert(key, source);
		}

		void removeSource(LinuxEventSource* source)
		{
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
			case Op::KIND_ACCEPT:
			case Op::KIND_READ:
				source->pushPollIn(key);
				break;
			case Op::KIND_WRITE:
				source->pushPollOut(key);
				break;
			default:
				assert(false);
				break;
			}

			m_pendingOps.insert(key, std::move(op));
		}

		Unique<Op> popPendingOp(Op* op)
		{
			auto it = m_pendingOps.lookup(op);
			if (it == m_pendingOps.end())
				return nullptr;
			auto res = std::move(it->value);
			m_pendingOps.remove(op);
			return res;
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

						if (auto err = source->handlePollIn()) return err;
					}

					if (event.events | EPOLLOUT)
					{
						if (auto err = source->handlePollOut()) return err;
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
			ZoneScoped;
			if (socket == nullptr)
				return nullptr;

			auto fd = (int)socket->fd();
			auto flags = fcntl(fd, F_GETFL, 0);
			if (flags == -1)
			{
				m_log->error("failed to get socket flags, ErrorCode({})"_sv, errno);
				return nullptr;
			}
			flags |= O_NONBLOCK;
			if (fcntl(fd, F_SETFL, flags) != 0)
			{
				m_log->error("failed to make socket non-blocking, ErrorCode({})"_sv, errno);
				return nullptr;
			}

			auto res = shared_from<LinuxSocketEventSource>(m_allocator, std::move(socket), this, m_allocator);

			epoll_event sourceSubscription{};
			sourceSubscription.events = EPOLLIN | EPOLLOUT | EPOLLET;
			sourceSubscription.data.ptr = res.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &sourceSubscription);
			if (ok == -1)
			{
				m_log->error("failed to create event source, ErrorCode({})"_sv, errno);
				return nullptr;
			}

			pushSource(res);
			return res;
		}
		HumanError read(EventSource* source, Reactor* reactor) override
		{
			ZoneScoped;
			pushPendingOp(unique_from<ReadOp>(m_allocator, reactor), (LinuxEventSource*)source);
			return {};
		}
		HumanError write(EventSource* source, Reactor* reactor, Span<const std::byte> buffer) override
		{
			Buffer ownedBuffer{m_allocator};
			ownedBuffer.push(buffer);
			pushPendingOp(unique_from<WriteOp>(m_allocator, reactor, std::move(ownedBuffer)), (LinuxEventSource*)source);
			return {};
		}
		HumanError accept(EventSource* source, Reactor* reactor) override
		{
			ZoneScoped;
			pushPendingOp(unique_from<AcceptOp>(m_allocator, reactor), (LinuxEventSource*)source);
			return {};
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