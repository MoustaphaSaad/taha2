#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Socket.h"

namespace core
{
	class WinOSEventLoop;
	class Reactor;

	class EventSource
	{
	public:
		virtual ~EventSource() = default;

		virtual HumanError read(WinOSEventLoop* loop, Reactor* reactor) = 0;
	};

	class Event
	{
	public:
		enum KIND
		{
			KIND_NONE,
			KIND_READ,
		};

		explicit Event(KIND kind)
			: m_kind(kind)
		{}

		virtual ~Event() = default;

		KIND kind() const { return m_kind; }
	private:
		KIND m_kind = KIND_NONE;
	};

	class ReadEvent: public Event
	{
		Span<const std::byte> m_buffer;
	public:
		ReadEvent(Span<const std::byte> buffer)
			: Event(Event::KIND_READ),
			  m_buffer(buffer)
		{}

		Span<const std::byte> buffer() const { return m_buffer; }
	};

	class Reactor
	{
	public:
		virtual ~Reactor() = default;

		virtual void handle(const Event* event)
		{
			switch (event->kind())
			{
			case Event::KIND_READ:
				onRead((const ReadEvent*)event);
				break;
			default:
				assert(false);
				break;
			}
		}

		virtual void onRead(const ReadEvent* event)
		{}
	};

	class EventLoop
	{
	public:
		CORE_EXPORT static Result<Unique<EventLoop>> create(Log* log, Allocator* allocator);

		virtual ~EventLoop() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;

		virtual Unique<EventSource> createEventSource(Unique<Socket> socket) = 0;
		virtual HumanError read(EventSource* source, Reactor* reactor) = 0;
	};
}