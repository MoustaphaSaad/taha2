#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Shared.h"
#include "core/Socket.h"

namespace core
{
	class Event
	{
	public:
		virtual ~Event() = default;
	};

	class StartEvent2: public Event
	{};

	class AcceptEvent2: public Event
	{
		Unique<Socket> m_socket;
	public:
		explicit AcceptEvent2(Unique<Socket> socket)
			: m_socket(std::move(socket))
		{}

		[[nodiscard]] Unique<Socket> releaseSocket() { return std::move(m_socket); }
	};

	class ReadEvent: public Event
	{
		Buffer m_backingBuffer;
		Span<const std::byte> m_bytes;
	public:
		ReadEvent(Span<const std::byte> bytes, Allocator* allocator)
			: m_backingBuffer(allocator),
			  m_bytes(bytes)
		{}

		explicit ReadEvent(Buffer&& bytes)
			: m_backingBuffer(std::move(bytes)),
			  m_bytes(m_backingBuffer)
		{}

		Span<const std::byte> bytes() const { return m_bytes; }
	};

	class WriteEvent: public Event
	{
		size_t m_writtenBytes = 0;
	public:
		explicit WriteEvent(size_t writtenBytes)
			: m_writtenBytes(writtenBytes)
		{}

		size_t writtenBytes() const { return m_writtenBytes; }
	};

	class EventThread;

	class EventSource
	{
	public:
		virtual ~EventSource() = default;

		virtual HumanError accept(const Shared<EventThread>& thread) = 0;
		virtual HumanError read(const Shared<EventThread>& thread) = 0;
		virtual HumanError write(Span<const std::byte> bytes, const Shared<EventThread>& thread) = 0;
	};

	class EventSocket
	{
		Shared<EventSource> m_socketSource;
	public:
		explicit EventSocket(const Shared<EventSource>& socket)
			: m_socketSource(socket)
		{}

		HumanError accept(const Shared<EventThread>& thread)
		{
			return m_socketSource->accept(thread);
		}

		HumanError read(const Shared<EventThread>& thread)
		{
			return m_socketSource->read(thread);
		}

		HumanError write(Span<const std::byte> bytes, const Shared<EventThread>& thread)
		{
			return m_socketSource->write(bytes, thread);
		}
	};

	class ThreadedEventLoop;

	class EventLoop
	{
		virtual void addThread(const Shared<EventThread>& thread) = 0;

	protected:
		Allocator* m_allocator = nullptr;
		ThreadedEventLoop* m_parentThreadedEventLoop = nullptr;
	public:
		CORE_EXPORT static Result<Unique<EventLoop>> create(ThreadedEventLoop* parent, Log* log, Allocator* allocator);
		static Result<Unique<EventLoop>> create(Log* log, Allocator* allocator)
		{
			return create(nullptr, log, allocator);
		}

		EventLoop(ThreadedEventLoop* threadedEventLoop, Allocator* allocator)
			: m_allocator(allocator),
			  m_parentThreadedEventLoop(threadedEventLoop)
		{}

		virtual ~EventLoop() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;
		virtual void stopAllLoops() = 0;
		virtual Result<EventSocket> registerSocket(Unique<Socket> socket) = 0;
		virtual EventLoop* next() = 0;

		template<typename T, typename ... TArgs>
		Shared<T> startThread(TArgs&& ... args)
		{
			auto thread = shared_from<T>(m_allocator, std::forward<TArgs>(args)...);
			addThread(thread);
			return thread;
		}
	};

	class EventThread: public SharedFromThis<EventThread>
	{
		EventLoop* m_eventLoop = nullptr;
		std::atomic_flag m_stopped;
	public:
		explicit EventThread(EventLoop* eventLoop)
			: m_eventLoop(eventLoop)
		{}

		virtual ~EventThread() = default;

		EventLoop* eventLoop() const { return m_eventLoop; }
		virtual HumanError handle(Event* event) = 0;
		CORE_EXPORT HumanError send(Unique<Event> event);
		CORE_EXPORT HumanError stop();
		CORE_EXPORT bool stopped() const;
	};
}