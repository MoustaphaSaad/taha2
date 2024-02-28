#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Shared.h"
#include "core/Socket.h"

namespace core
{
	class Event2
	{
	public:
		virtual ~Event2() = default;
	};

	class StartEvent2: public Event2
	{};

	class AcceptEvent2: public Event2
	{
		Unique<Socket> m_socket;
	public:
		explicit AcceptEvent2(Unique<Socket> socket)
			: m_socket(std::move(socket))
		{}

		[[nodiscard]] Unique<Socket> releaseSocket() { return std::move(m_socket); }
	};

	class ReadEvent2: public Event2
	{
		Span<const std::byte> m_bytes;
	public:
		explicit ReadEvent2(Span<const std::byte> bytes)
			: m_bytes(bytes)
		{}

		Span<const std::byte> bytes() const { return m_bytes; }
	};

	class WriteEvent2: public Event2
	{
		size_t m_writtenBytes = 0;
	public:
		explicit WriteEvent2(size_t writtenBytes)
			: m_writtenBytes(writtenBytes)
		{}

		size_t writtenBytes() const { return m_writtenBytes; }
	};

	class EventThread2;

	class EventSource2
	{
	public:
		virtual ~EventSource2() = default;

		virtual HumanError accept(const Shared<EventThread2>& thread) = 0;
		virtual HumanError read(const Shared<EventThread2>& thread) = 0;
		virtual HumanError write(Span<const std::byte> bytes, const Shared<EventThread2>& thread) = 0;
	};

	class EventSocket2
	{
		Shared<EventSource2> m_socketSource;
	public:
		explicit EventSocket2(const Shared<EventSource2>& socket)
			: m_socketSource(socket)
		{}

		HumanError accept(const Shared<EventThread2>& thread)
		{
			return m_socketSource->accept(thread);
		}

		HumanError read(const Shared<EventThread2>& thread)
		{
			return m_socketSource->read(thread);
		}

		HumanError write(Span<const std::byte> bytes, const Shared<EventThread2>& thread)
		{
			return m_socketSource->write(bytes, thread);
		}
	};

	class EventLoop2
	{
		virtual void addThread(const Shared<EventThread2>& thread) = 0;

	protected:
		Allocator* m_allocator = nullptr;

	public:
		CORE_EXPORT static Result<Unique<EventLoop2>> create(Log* log, Allocator* allocator);

		explicit EventLoop2(Allocator* allocator)
			: m_allocator(allocator)
		{}

		virtual ~EventLoop2() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;
		virtual Result<EventSocket2> registerSocket(Unique<Socket> socket) = 0;

		template<typename T, typename ... TArgs>
		Shared<T> startThread(TArgs&& ... args)
		{
			auto thread = shared_from<T>(m_allocator, std::forward<TArgs>(args)...);
			addThread(thread);
			return thread;
		}
	};

	class EventThread2: public SharedFromThis<EventThread2>
	{
		EventLoop2* m_eventLoop = nullptr;
	public:
		explicit EventThread2(EventLoop2* eventLoop)
			: m_eventLoop(eventLoop)
		{}

		virtual HumanError handle(Event2* event) = 0;
		CORE_EXPORT HumanError send(Unique<Event2> event);
		CORE_EXPORT void stop();
	};
}