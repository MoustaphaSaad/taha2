#include "core/EventLoop.h"
#include "core/Hash.h"
#include "core/Queue.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <fcntl.h>

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
				KIND_READ,
				KIND_WRITE,
				KIND_ACCEPT,
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
			{};
		};

		struct AcceptOp: Op
		{
			Reactor* reactor = nullptr;

			AcceptOp(Reactor* reactor_)
				: Op(KIND_ACCEPT),
				  reactor(reactor_)
			{}
		};

		class LinuxEventSource: public EventSource
		{
			Queue<Op*> m_pollInOps;
			Queue<Op*> m_pollOutOps;
		public:
			enum OP_RESULT
			{
				OP_RESULT_OK,
				OP_RESULT_EAGAIN,
				OP_RESULT_ECONNRESET,
				OP_RESULT_PARTIAL_WRITE,
			};
			explicit LinuxEventSource(EventLoop* loop, Allocator* allocator)
				: EventSource(loop),
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

			void pushPollOut(Op* op)
			{
				m_pollOutOps.push_back(op);
			}

			void pushPollOutToFront(Op* op)
			{
				m_pollOutOps.push_front(op);
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

			virtual Result<OP_RESULT> readReady(ReadOp* op) = 0;
			virtual Result<OP_RESULT> writeReady(WriteOp* op) = 0;
			virtual Result<OP_RESULT> acceptReady(AcceptOp* op) = 0;
		};

		class LinuxCloseEventSource: public LinuxEventSource
		{
			Allocator* m_allocator = nullptr;
			int m_eventfd = 0;
		public:
			LinuxCloseEventSource(int eventfd, EventLoop* loop, Allocator* allocator)
				: LinuxEventSource(loop, allocator),
				  m_allocator(allocator),
				  m_eventfd(eventfd)
			{}

			~LinuxCloseEventSource()
			{
				if (m_eventfd)
				{
					[[maybe_unused]] auto res = ::close(m_eventfd);
					assert(res == 0);
				}
			}

			void signalClose()
			{
				int64_t close = 1;
				[[maybe_unused]] auto res = ::write(m_eventfd, &close, sizeof(close));
				assert(res == sizeof(close));
			}

			Result<OP_RESULT> readReady(ReadOp*) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}

			Result<OP_RESULT> writeReady(WriteOp*) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}

			Result<OP_RESULT> acceptReady(AcceptOp*) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}
		};

		class LinuxSocketEventSource: public LinuxEventSource
		{
			Allocator* m_allocator = nullptr;
			Unique<Socket> m_socket;
			std::byte line[2048] = {};
		public:
			LinuxSocketEventSource(Unique<Socket> socket, EventLoop* loop, Allocator* allocator)
				: LinuxEventSource(loop, allocator),
				  m_allocator(allocator),
				  m_socket(std::move(socket))
			{}

			~LinuxSocketEventSource() override
			{
				m_socket->shutdown(Socket::SHUT_RDWR);
			}

			Result<OP_RESULT> readReady(ReadOp* op) override
			{
				auto readBytes = ::read(m_socket->fd(), line, sizeof(line));
				if (readBytes == -1)
				{
					if (errno == EAGAIN || errno == EWOULDBLOCK)
					{
						return OP_RESULT_EAGAIN;
					}
					else if (errno == ECONNRESET)
					{
						ReadEvent readEvent{{}, this};
						op->reactor->handle(&readEvent);
						return OP_RESULT_ECONNRESET;
					}

					return errf(m_allocator, "failed to read from socket, ErrorCode({})"_sv, errno);
				}
				if (op->reactor)
				{
					ReadEvent readEvent{Span<const std::byte>{line, (size_t) readBytes}, this};
					op->reactor->handle(&readEvent);
				}
				return OP_RESULT_OK;
			}

			Result<OP_RESULT> writeReady(WriteOp* op) override
			{
				auto writtenBytes = ::write(m_socket->fd(), op->remainingBytes.data(), op->remainingBytes.count());
				if (writtenBytes == -1)
				{
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						return OP_RESULT_EAGAIN;
					else if (errno == ECONNRESET)
						return OP_RESULT_ECONNRESET;

					return errf(m_allocator, "failed to write to socket, ErrorCode({})"_sv, errno);
				}
				if (op->reactor)
				{
					WriteEvent writeEvent{(size_t)writtenBytes, this};
					op->reactor->handle(&writeEvent);
				}
				op->remainingBytes = op->remainingBytes.slice(writtenBytes, op->remainingBytes.count() - writtenBytes);

				if (op->remainingBytes.count() > 0)
					return OP_RESULT_PARTIAL_WRITE;
				return OP_RESULT_OK;
			}

			Result<OP_RESULT> acceptReady(AcceptOp* op) override
			{
				auto acceptedSocket = m_socket->accept();
				if (acceptedSocket == nullptr)
				{
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						return OP_RESULT_EAGAIN;
					else if (errno == ECONNRESET)
						return OP_RESULT_ECONNRESET;

					return errf(m_allocator, "failed to accept from socket, ErrorCode({})"_sv, errno);
				}
				if (op->reactor)
				{
					AcceptEvent acceptEvent{std::move(acceptedSocket), this};
					op->reactor->handle(&acceptEvent);
				}
				return OP_RESULT_OK;
			}
		};

		void removeSource(LinuxEventSource* source)
		{
			m_sources.remove(source);
		}

		Shared<LinuxEventSource> getSourceAndRemoveItWhenExpired(LinuxEventSource* source)
		{
			auto it = m_sources.lookup(source);
			if (it == m_sources.end())
				return nullptr;
			auto& weakHandle = it->value;
			if (weakHandle.expired())
			{
				m_sources.remove(source);
				return nullptr;
			}

			return weakHandle.lock();
		}

		void continueWriteOp(Unique<WriteOp> op, LinuxEventSource* source)
		{
			assert(op->remainingBytes.count() > 0);

			auto key = (Op*)op.get();

			source->pushPollOutToFront(key);

			m_scheduledOperations.insert(key, std::move(op));
		}

		void pushPendingOp(Unique<Op> op, LinuxEventSource* source)
		{
			auto key = (Op*)op.get();

			switch (op->kind)
			{
			case Op::KIND_CLOSE:
			case Op::KIND_READ:
			case Op::KIND_ACCEPT:
				source->pushPollIn(key);
				break;
			case Op::KIND_WRITE:
				source->pushPollOut(key);
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

		Result<bool> handleOp(Op* opHandle, LinuxEventSource* source)
		{
			if (opHandle == nullptr)
				return false;

			auto op = popPendingOp((Op*)opHandle);
			assert(op != nullptr);

			switch (op->kind)
			{
			case Op::KIND_CLOSE:
				m_log->debug("event loop closed"_sv);
				m_closeEventSource = nullptr;
				m_sources.clear();
				m_scheduledOperations.clear();
				return true;
			case Op::KIND_READ:
			{
				auto readOp = unique_static_cast<ReadOp>(std::move(op));
				auto opResult = source->readReady(readOp.get());
				if (opResult.isError()) return opResult.releaseError();
				auto res = opResult.releaseValue();
				if (res == LinuxEventSource::OP_RESULT_EAGAIN)
					pushPendingOp(std::move(readOp), source);
				else if (res == LinuxEventSource::OP_RESULT_ECONNRESET)
					m_sources.remove(source);
				return false;
			}
			case Op::KIND_WRITE:
			{
				auto writeOp = unique_static_cast<WriteOp>(std::move(op));
				auto opResult = source->writeReady(writeOp.get());
				if (opResult.isError()) return opResult.releaseError();
				auto res = opResult.releaseValue();
				if (res == LinuxEventSource::OP_RESULT_EAGAIN)
					pushPendingOp(std::move(writeOp), source);
				else if (res == LinuxEventSource::OP_RESULT_PARTIAL_WRITE)
					continueWriteOp(std::move(writeOp), source);
				else if (res == LinuxEventSource::OP_RESULT_ECONNRESET)
					m_sources.remove(source);
				return false;
			}
			case Op::KIND_ACCEPT:
			{
				auto acceptOp = unique_static_cast<AcceptOp>(std::move(op));
				auto opResult = source->acceptReady(acceptOp.get());
				if (opResult.isError()) return opResult.releaseError();
				auto res = opResult.releaseValue();
				if (res == LinuxEventSource::OP_RESULT_EAGAIN)
					pushPendingOp(std::move(acceptOp), source);
				else if (res == LinuxEventSource::OP_RESULT_ECONNRESET)
					m_sources.remove(source);
				return false;
			}
			default:
				assert(false);
				return errf(m_allocator, "unknown event loop event kind, {}"_sv, (int)op->kind);
			}
		}

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		int m_epoll = 0;
		Shared<LinuxCloseEventSource> m_closeEventSource;
		Map<Op*, Unique<Op>> m_scheduledOperations;
		Map<LinuxEventSource*, Weak<LinuxEventSource>> m_sources;
	public:
		LinuxEventLoop(int epoll, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_scheduledOperations(allocator),
			  m_sources(allocator)
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
			m_sources.insert(m_closeEventSource.get(), m_closeEventSource);

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
						return {};
					}
					else
					{
						return errf(m_allocator, "epoll_wait failed, ErrorCode({})"_sv, errno);
					}
				}

				for (int i = 0; i < count; ++i)
				{
					auto event = events[i];
					auto sourceHandle = (LinuxEventSource*) event.data.ptr;

					auto source = getSourceAndRemoveItWhenExpired(sourceHandle);
					if (source == nullptr)
					{
						m_log->debug("expired source: {}, sources count: {}"_sv, (void*)sourceHandle, m_sources.count());
						continue;
					}

					if (event.events | EPOLLIN)
					{
						auto op = source->popPollIn();
						auto shouldCloseResult = handleOp(op, source.get());
						if (shouldCloseResult.isError()) return shouldCloseResult.releaseError();
						auto shouldClose = shouldCloseResult.releaseValue();
						if (shouldClose) return {};
					}

					if (event.events | EPOLLOUT)
					{
						auto op = source->popPollOut();
						auto shouldCloseResult = handleOp(op, source.get());
						if (shouldCloseResult.isError()) return shouldCloseResult.releaseError();
						auto shouldClose = shouldCloseResult.releaseValue();
						if (shouldClose) return {};
					}
				}
			}

			return {};
		}

		void stop() override
		{
			m_log->debug("stop"_sv);
			m_closeEventSource->signalClose();
		}

		Shared<EventSource> createEventSource(Unique<Socket> socket) override
		{
			if (socket == nullptr)
				return nullptr;

			auto fd = (int)socket->fd();
			auto flags = fcntl(fd, F_GETFL, 0);
			if (flags == -1)
			{
				m_log->error("failed to get socket flags"_sv);
				return nullptr;
			}
			flags |= O_NONBLOCK;
			if (fcntl(fd, F_SETFL, flags) != 0)
			{
				m_log->error("failed to make socket non-blocking"_sv);
				return nullptr;
			}

			auto res = shared_from<LinuxSocketEventSource>(m_allocator, std::move(socket), this, m_allocator);

			epoll_event event{};
			event.events = EPOLLIN | EPOLLOUT;
			event.data.ptr = res.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &event);
			if (ok == -1)
			{
				m_log->error("failed to create event source"_sv);
				return nullptr;
			}

			auto sourceKey = res.get();
			m_sources.insert(sourceKey, res);
			return res;
		}

		HumanError read(EventSource* source, Reactor* reactor) override
		{
			auto op = unique_from<ReadOp>(m_allocator, reactor);
			pushPendingOp(std::move(op), (LinuxEventSource*)source);
			return {};
		}

		HumanError write(EventSource* source, Reactor* reactor, Span<const std::byte> buffer) override
		{
			Buffer ownedBuffer{m_allocator};
			ownedBuffer.push(buffer);
			auto op = unique_from<WriteOp>(m_allocator, reactor, std::move(ownedBuffer));
			pushPendingOp(std::move(op), (LinuxEventSource*)source);
			return {};
		}

		HumanError accept(EventSource* source, Reactor* reactor) override
		{
			auto op = unique_from<AcceptOp>(m_allocator, reactor);
			pushPendingOp(std::move(op), (LinuxEventSource*)source);
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