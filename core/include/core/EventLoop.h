#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Socket.h"

namespace core
{
	class EventLoop;

	class EventSource
	{
		EventLoop* m_eventLoop = nullptr;
	public:
		explicit EventSource(EventLoop* eventLoop)
			: m_eventLoop(eventLoop)
		{}

		virtual ~EventSource() = default;

		EventLoop* eventLoop() const { return m_eventLoop; }
	};

	class Event
	{
	public:
		enum KIND
		{
			KIND_NONE,
			KIND_READ,
			KIND_ACCEPT,
			KIND_WRITE,
		};

		explicit Event(KIND kind, EventSource* source)
			: m_kind(kind),
			  m_source(source)
		{}

		virtual ~Event() = default;

		KIND kind() const { return m_kind; }
		EventSource* source() const { return m_source; }
		EventLoop* eventLoop() const { return m_source->eventLoop(); }
	private:
		KIND m_kind = KIND_NONE;
		EventSource* m_source = nullptr;
	};

	class ReadEvent: public Event
	{
		Span<const std::byte> m_buffer;
	public:
		ReadEvent(Span<const std::byte> buffer, EventSource* source)
			: Event(Event::KIND_READ, source),
			  m_buffer(buffer)
		{}

		Span<const std::byte> buffer() const { return m_buffer; }
	};

	class AcceptEvent: public Event
	{
		Unique<Socket> m_socket;
	public:
		AcceptEvent(Unique<Socket> socket, EventSource* source)
			: Event(Event::KIND_ACCEPT, source),
			  m_socket(std::move(socket))
		{}

		Unique<Socket> releaseSocket() { return std::move(m_socket); }
	};

	class WriteEvent: public Event
	{
		size_t m_writtenSize = 0;
	public:
		WriteEvent(size_t writtenSize, EventSource* source)
			: Event(Event::KIND_WRITE, source),
			  m_writtenSize(writtenSize)
		{}

		size_t writtenSize() const { return m_writtenSize; }
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
			case Event::KIND_ACCEPT:
				onAccept((AcceptEvent*)event);
				break;
			case Event::KIND_WRITE:
				onWrite((const WriteEvent*)event);
				break;
			default:
				assert(false);
				break;
			}
		}

		virtual void onRead(const ReadEvent* event)
		{}

		virtual void onAccept(AcceptEvent* event)
		{}

		virtual void onWrite(const WriteEvent* event)
		{}
	};

	class EventLoop
	{
	public:
		CORE_EXPORT static Result<Unique<EventLoop>> create(Log* log, Allocator* allocator);

		virtual ~EventLoop() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;

		virtual Shared<EventSource> createEventSource(Unique<Socket> socket) = 0;
		virtual HumanError read(EventSource* source, Reactor* reactor) = 0;
		virtual HumanError write(EventSource* source, Reactor* reactor, Span<const std::byte> buffer) = 0;
		virtual HumanError accept(EventSource* source, Reactor* reactor) = 0;
	};
}