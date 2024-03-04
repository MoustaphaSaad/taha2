#include "core/websocket/Server2.h"
#include "core/websocket/Handshake.h"
#include "core/SHA1.h"
#include "core/Base64.h"

namespace core::websocket
{
	class HandshakeThread: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Shared<EventThread2> m_handler;
		Buffer m_buffer;
		size_t m_maxHandshakeSize = 0;
	public:
		HandshakeThread(EventSocket2 socket, const Shared<EventThread2>& handler, size_t maxHandshakeSize, EventLoop2* loop, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_handler(handler),
			  m_buffer(allocator),
			  m_maxHandshakeSize(maxHandshakeSize)
		{}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				return m_socket.read(sharedFromThis());
			}
			else if (auto readEvent = dynamic_cast<ReadEvent2*>(event))
			{
				auto totalHandshakeBuffer = m_buffer.count() + readEvent->bytes().count();
				if (totalHandshakeBuffer > m_maxHandshakeSize)
				{
					// TODO: maybe wait until data is sent
					m_log->error("handshake size is too long, size: {}bytes, max: {}bytes"_sv, totalHandshakeBuffer, m_maxHandshakeSize);
					(void)m_socket.write(R"(HTTP/1.1 400 Invalid\r\nerror: too large\r\ncontent-length: 0\r\n\r\n)"_sv, nullptr);
					return stop();
				}
				m_buffer.push(readEvent->bytes());

				auto handshakeString = StringView{m_buffer};
				if (handshakeString.endsWith("\r\n\r\n"_sv))
				{
					auto handshakeResult = Handshake::parse(handshakeString, m_allocator);
					if (handshakeResult.isError())
					{
						// TODO: maybe wait until data is sent
						(void)m_socket.write(R"(HTTP/1.1 400 Invalid\r\nerror: failed to parse handshake\r\ncontent-length: 0\r\n\r\n)"_sv, nullptr);
						return stop();
					}
					auto handshake = handshakeResult.releaseValue();

					constexpr static const char* REPLY = "HTTP/1.1 101 Switching Protocols\r\n"
						"Upgrade: websocket\r\n"
						"Connection: Upgrade\r\n"
						"Sec-WebSocket-Accept: {}\r\n"
						"\r\n";
					auto concatKey = strf(m_allocator, "{}{}"_sv, handshake.key(), "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
					auto sha1 = SHA1::hash(concatKey);
					auto base64 = Base64::encode(sha1.asBytes(), m_allocator);
					auto reply = strf(m_allocator, StringView{REPLY, strlen(REPLY)}, base64);
					if (auto err = m_socket.write(Span<const std::byte>{reply}, nullptr))
					{
						m_log->error("failed to send handshake reply, {}"_sv, err);
						return stop();
					}

					auto newConn = unique_from<NewConnection>(m_allocator, m_socket);
					if (auto err = m_handler->send(std::move(newConn)))
					{
						m_log->error("failed to send new connection event, {}"_sv, err);
						return stop();
					}
					return stop();
				}
				else
				{
					return m_socket.read(sharedFromThis());
				}
			}
			return {};
		}
	};

	class AcceptThread: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Shared<EventThread2> m_handler;
		Array<Shared<EventThread2>> m_threads;
		size_t m_maxHandshakeSize;
	public:
		explicit AcceptThread(EventSocket2 socket, Shared<EventThread2> handler, size_t maxHandshakeSize, EventLoop2* eventLoop, Log* log, Allocator* allocator)
			: EventThread2(eventLoop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_handler(handler),
			  m_threads(allocator),
			  m_maxHandshakeSize(maxHandshakeSize)
		{}

		~AcceptThread() override
		{
			for (auto thread: m_threads)
				(void)thread->stop();
		}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				return m_socket.accept(sharedFromThis());
			}
			else if (auto acceptEvent = dynamic_cast<AcceptEvent2*>(event))
			{
				auto loop = eventLoop()->next();
				auto socketResult = loop->registerSocket(acceptEvent->releaseSocket());
				if (socketResult.isError())
					return socketResult.releaseError();

				auto handshake = loop->startThread<HandshakeThread>(socketResult.releaseValue(), m_handler, m_maxHandshakeSize, loop, m_log, m_allocator);
				m_threads.push(handshake);
				return m_socket.accept(sharedFromThis());
			}
			return {};
		}
	};

	HumanError Server2::start(const ServerConfig2 &config, EventLoop2 *loop)
	{
		auto socket = Socket::open(m_allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
		if (socket == nullptr)
			return errf(m_allocator, "failed to open accept socket"_sv);

		auto ok = socket->bind(config.host, config.port);
		if (ok == false)
			return errf(m_allocator, "failed to bind socket"_sv);

		ok = socket->listen();
		if (ok == false)
			return errf(m_allocator, "failed to listen socket"_sv);

		auto eventSocketResult = loop->registerSocket(std::move(socket));
		if (eventSocketResult.isError())
			return eventSocketResult.releaseError();
		auto eventSocket = eventSocketResult.releaseValue();

		m_acceptThread = loop->startThread<AcceptThread>(eventSocket, config.handler, config.maxHandshakeSize, loop, m_log, m_allocator);
		return {};
	}
}