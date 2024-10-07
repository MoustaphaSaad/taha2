#include "core/ws/Client.h"
#include "core/Base64.h"
#include "core/Lock.h"
#include "core/MemoryStream.h"
#include "core/Rand.h"
#include "core/SHA1.h"
#include "core/Url.h"
#include "core/ws/Handshake.h"

namespace core::ws
{
	HumanError Client::write(Span<const std::byte> bytes)
	{
		if (bytes.sizeInBytes() == 0)
		{
			return {};
		}

		auto writtenSize = m_socket->write(bytes.data(), bytes.sizeInBytes());
		if (writtenSize != bytes.sizeInBytes())
		{
			return errf(
				m_allocator, "failed to write into socket, wrote {}/{} bytes"_sv, writtenSize, bytes.sizeInBytes());
		}
		return {};
	}

	HumanError Client::read(Span<std::byte> bytes)
	{
		auto readSize = m_bufferedReader.read(bytes.data(), bytes.count());
		if (readSize != bytes.sizeInBytes())
		{
			return errf(m_allocator, "failed to read from socket, read {}/{} bytes"_sv, readSize, bytes.count());
		}
		return {};
	}

	HumanError Client::sendHandshake(const Url& url, const String& base64Key)
	{
		auto pathResult = url.pathWithQueryAndFragment(m_allocator);
		if (pathResult.isError())
		{
			return pathResult.releaseError();
		}
		auto path = pathResult.releaseValue();

		MemoryStream request{m_allocator};
		strf(&request, "GET {} HTTP/1.1\r\n"_sv, path);
		strf(&request, "content-length: 0\r\n"_sv);
		strf(&request, "upgrade: websocket\r\n"_sv);
		strf(&request, "sec-websocket-version: 13\r\n"_sv);
		strf(&request, "connection: upgrade\r\n"_sv);
		strf(&request, "sec-websocket-key: {}\r\n"_sv, base64Key);
		strf(&request, "Host: {}:{}\r\n"_sv, url.host(), url.port());
		strf(&request, "\r\n"_sv);

		auto requestAsString = request.releaseString();
		return write(Span<const std::byte>{requestAsString});
	}

	Result<String> Client::readHTTP(size_t maxSize)
	{
		StringView http;
		while (http.find("\r\n\r\n"_sv) == SIZE_MAX && http.count() < maxSize)
		{
			http = m_bufferedReader.peek(http.count() + 1024);
		}

		if (http.count() > maxSize)
		{
			return errf(m_allocator, "http size is too large, >{}"_sv, maxSize);
		}

		auto httpEnd = http.find("\r\n\r\n"_sv);
		String httpString{m_allocator};
		// +4 for the \r\n\r\n at the end
		httpString.resize(httpEnd + 4);
		auto err = read(StringView{httpString});
		if (err)
		{
			return errf(m_allocator, "failed to read http request or response, {}"_sv, err);
		}
		return httpString;
	}

	HumanError Client::handshake(const Url& url)
	{
		std::byte rawKey[16] = {};
		Span<std::byte> key{rawKey, sizeof(rawKey)};
		auto ok = Rand::cryptoRand(key);
		if (ok == false)
		{
			return errf(m_allocator, "failed to generate random key"_sv);
		}

		auto base64Key = Base64::encode(key, m_allocator);
		auto err = sendHandshake(url, base64Key);
		if (err)
		{
			return err;
		}

		// read response
		auto httpResponseResult = readHTTP(m_maxHandshakeSize);
		if (httpResponseResult.isError())
		{
			return httpResponseResult.releaseError();
		}
		auto httpResponse = httpResponseResult.releaseValue();

		auto handshakeResult = Handshake::parseResponse(httpResponse, m_allocator);
		if (handshakeResult.isError())
		{
			return handshakeResult.releaseError();
		}
		auto handshake = handshakeResult.releaseValue();

		SHA1Hasher hasher;
		hasher.hash(base64Key);
		hasher.hash("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
		auto expectedBase64 = Base64::encode(hasher.final().asBytes(), m_allocator);
		if (expectedBase64 != handshake.key())
		{
			return errf(m_allocator, "invalid websocket accept header"_sv);
		}
		return {};
	}

	HumanError Client::serverHandshake()
	{
		auto httpRequestResult = readHTTP(m_maxHandshakeSize);
		if (httpRequestResult.isError())
		{
			return httpRequestResult.releaseError();
		}
		auto httpRequest = httpRequestResult.releaseValue();

		auto handshakeResult = Handshake::parse(httpRequest, m_allocator);
		if (handshakeResult.isError())
		{
			constexpr static auto REPLY =
				R"(HTTP/1.1 400 Invalid\r\nerror: failed to parse handshake\r\ncontent-length: 0\r\n\r\n)";
			(void)write(StringView{REPLY});
			return handshakeResult.releaseError();
		}
		auto handshake = handshakeResult.releaseValue();

		constexpr static const char* REPLY = "HTTP/1.1 101 Switching Protocols\r\n"
											 "Upgrade: websocket\r\n"
											 "Connection: Upgrade\r\n"
											 "Sec-WebSocket-Accept: {}\r\n"
											 "\r\n";

		SHA1Hasher hasher;
		hasher.hash(handshake.key());
		hasher.hash("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
		auto base64 = Base64::encode(hasher.final().asBytes(), m_allocator);
		auto reply = strf(m_allocator, StringView{REPLY}, base64);
		return write(StringView{reply});
	}

	HumanError Client::writeFrame(Frame::OPCODE opcode, Span<const std::byte> payload)
	{
		if (Frame::isControlOpcode(opcode))
		{
			assertMsg(payload.sizeInBytes() <= 125, "control frames are limited to 125 bytes");
			// limit it to 125 bytes
			if (payload.sizeInBytes() > 125)
			{
				payload = payload.sliceLeft(125);
			}
		}

		std::byte rawMask[4] = {};
		Span<std::byte> mask{rawMask, sizeof(rawMask)};
		if (m_shouldMask)
		{
			auto ok = Rand::cryptoRand(mask);
			if (ok == false)
			{
				return errf(m_allocator, "failed to generate mask"_sv);
			}
		}

		std::byte buf[14] = {};
		buf[0] = std::byte(128 | opcode);
		size_t buf_size = 1;
		if (payload.sizeInBytes() <= 125)
		{
			buf[1] = std::byte(payload.sizeInBytes());
			++buf_size;
		}
		else if (payload.sizeInBytes() <= UINT16_MAX)
		{
			buf[1] = std::byte(126);
			buf[2] = std::byte((payload.sizeInBytes() >> 8) & 0xFF);
			buf[3] = std::byte(payload.sizeInBytes() & 0xFF);
			buf_size += 3;
		}
		else
		{
			buf[1] = std::byte(127);
			buf[2] = std::byte((payload.sizeInBytes() >> 56) & 0xFF);
			buf[3] = std::byte((payload.sizeInBytes() >> 48) & 0xFF);
			buf[4] = std::byte((payload.sizeInBytes() >> 40) & 0xFF);
			buf[5] = std::byte((payload.sizeInBytes() >> 32) & 0xFF);
			buf[6] = std::byte((payload.sizeInBytes() >> 24) & 0xFF);
			buf[7] = std::byte((payload.sizeInBytes() >> 16) & 0xFF);
			buf[8] = std::byte((payload.sizeInBytes() >> 8) & 0xFF);
			buf[9] = std::byte(payload.sizeInBytes() & 0xFF);
			buf_size += 9;
		}

		if (m_shouldMask)
		{
			buf[1] = std::byte(uint8_t(buf[1]) | 128);
			::memcpy(buf + buf_size, mask.data(), mask.sizeInBytes());
			buf_size += mask.sizeInBytes();
		}

		// write header
		if (auto err = write(Span<const std::byte>{buf, buf_size}); err)
		{
			return err;
		}

		if (m_shouldMask)
		{
			Buffer maskedPayload{m_allocator};
			maskedPayload.push(payload);
			for (size_t i = 0; i < maskedPayload.count(); ++i)
			{
				maskedPayload[i] ^= mask[i & 3];
			}
			return write(Span<const std::byte>{maskedPayload});
		}
		else
		{
			return write(payload);
		}
	}

	HumanError Client::writeCloseWithCode(uint16_t code, StringView reason)
	{
		std::byte buf[125] = {};
		buf[0] = std::byte((code >> 8) & 0xFF);
		buf[1] = std::byte(code & 0xFF);
		auto maxReasonSize = reason.count() > 123 ? 0 : reason.count();
		::memcpy(buf + 2, reason.data(), maxReasonSize);
		auto payloadSize = maxReasonSize + 2;
		return writeFrame(Frame::OPCODE_CLOSE, Span<const std::byte>{buf, payloadSize});
	}

	Result<Client>
	Client::connect(StringView url, size_t maxHandshakeSize, size_t maxMessageSize, Log* log, Allocator* allocator)
	{
		auto parsedUrlResult = Url::parse(url, allocator);
		if (parsedUrlResult.isError())
		{
			return parsedUrlResult.releaseError();
		}
		auto parsedUrl = parsedUrlResult.releaseValue();

		auto socket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
		if (socket == nullptr)
		{
			return core::errf(allocator, "failed to open a socket"_sv);
		}

		auto ok = socket->connect(parsedUrl.host(), parsedUrl.port());
		if (ok == false)
		{
			return core::errf(allocator, "failed to connect to '{}'"_sv, url);
		}

		Client client{std::move(socket), maxHandshakeSize, maxMessageSize, log, allocator};
		if (auto err = client.handshake(parsedUrl); err)
		{
			return err;
		}

		client.m_shouldMask = true;
		return client;
	}

	Result<Client> Client::acceptFromServer(
		Unique<Socket> socket, size_t maxHandshakeSize, size_t maxMessageSize, Log* log, Allocator* allocator)
	{
		Client client{std::move(socket), maxHandshakeSize, maxMessageSize, log, allocator};
		if (auto err = client.serverHandshake(); err)
		{
			return err;
		}
		return client;
	}

	Result<Message> Client::readMessage()
	{
		auto messageResult = m_messageParser.read(&m_bufferedReader, m_allocator);
		// write error event here
		if (messageResult.isError())
		{
			(void)writeClose(1002, messageResult.error().message());
			return messageResult.releaseError();
		}
		auto message = messageResult.releaseValue();

		// write error event here
		if (message.kind == Message::KIND_TEXT && StringView{message.payload}.isValidUtf8() == false)
		{
			(void)writeClose(1007, "invalid utf8 string in text payload"_sv);
			return errf(m_allocator, "invalid utf8 string in text payload"_sv);
		}

		return message;
	}

	HumanError Client::handleMessage(const Message& message)
	{
		switch (message.kind)
		{
		case Message::KIND_CLOSE:
		{
			if (message.payload.count() == 0)
			{
				return writeClose(1000, ""_sv);
			}

			// error protocol close payload should have at least 2 byte
			if (message.payload.count() == 1)
			{
				return writeClose(1002, "close payload should have at least 2 byte"_sv);
			}

			auto errorCode = uint16_t(message.payload[1]) | (uint16_t(message.payload[0]) << 8);
			if (errorCode < 1000 || errorCode == 1004 || errorCode == 1005 || errorCode == 1006 ||
				(errorCode > 1013 && errorCode < 3000))
			{
				return writeClose(1002, ""_sv);
			}

			if (message.payload.count() == 2)
			{
				return writeClose(1000, ""_sv);
			}

			auto payload = StringView{message.payload}.sliceRight(2);
			if (payload.isValidUtf8() == false)
			{
				return writeClose(1007, "invalid utf8 in close reason"_sv);
			}

			return writeClose(1000, ""_sv);
		}
		case Message::KIND_PING:
		{
			return writePong(Span<const std::byte>{message.payload});
		}
		case Message::KIND_PONG:
		case Message::KIND_TEXT:
		case Message::KIND_BINARY:
			return {};
		default:
			unreachable();
			return errf(m_allocator, "unknown message kind"_sv);
		}
	}

	HumanError Client::writeText(StringView payload)
	{
		auto lock = lockGuard(m_writeMutex);
		return writeFrame(Frame::OPCODE_TEXT, payload);
	}

	HumanError Client::writeBinary(Span<const std::byte> payload)
	{
		auto lock = lockGuard(m_writeMutex);
		return writeFrame(Frame::OPCODE_BINARY, payload);
	}

	HumanError Client::writePing(Span<const std::byte> payload)
	{
		auto lock = lockGuard(m_writeMutex);
		return writeFrame(Frame::OPCODE_PING, payload);
	}

	HumanError Client::writePong(Span<const std::byte> payload)
	{
		auto lock = lockGuard(m_writeMutex);
		return writeFrame(Frame::OPCODE_PONG, payload);
	}

	HumanError Client::writeClose(uint16_t code, StringView reason)
	{
		auto lock = lockGuard(m_writeMutex);
		return writeCloseWithCode(code, reason);
	}

	void Client::close()
	{
		m_socket->shutdown(Socket::SHUTDOWN_RDWR);
		m_socket->close();
	}
}