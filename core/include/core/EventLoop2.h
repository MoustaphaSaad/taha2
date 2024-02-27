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