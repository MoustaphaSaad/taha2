#include "core/Socket.h"
#include "core/String.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <netdb.h>

#include <assert.h>

namespace core
{
	class LinuxSocket: public Socket
	{
		Allocator* m_allocator = nullptr;
		int m_handle = -1;
		int m_family = 0;
		int m_type = 0;
		int m_protocol = 0;
	public:
		LinuxSocket(Allocator* allocator, int handle, int family, int type, int protocol)
			: m_allocator(allocator),
			  m_handle(handle),
			  m_family(family),
			  m_type(type),
			  m_protocol(protocol)
		{}

		bool close() override
		{
			if (m_handle == -1)
				return false;

			auto err = ::close(m_handle);
			if (err == -1)
				return false;

			m_handle = -1;
			return true;
		}

		bool connect(StringView address, StringView port) override
		{
			addrinfo hints = {};
			hints.ai_family = m_family;
			hints.ai_socktype = m_type;
			hints.ai_protocol = m_protocol;

			String c_address(address, m_allocator);
			String c_port(port, m_allocator);
			addrinfo* result = nullptr;
			auto err = ::getaddrinfo(c_address.data(), c_port.data(), &hints, &result);
			if (err != 0)
				return false;

			for (auto it = result; it; it = it->ai_next)
			{
				err = ::connect(m_handle, it->ai_addr, (int)it->ai_addrlen);
				if (err == 0)
				{
					::freeaddrinfo(result);
					return true;
				}
			}

			::freeaddrinfo(result);
			return false;
		}

		bool bind(StringView port) override
		{
			addrinfo hints = {};
			hints.ai_family = m_family;
			hints.ai_socktype = m_type;
			hints.ai_protocol = m_protocol;
			hints.ai_flags = AI_PASSIVE;

			String c_port(port, m_allocator);
			addrinfo* result = nullptr;
			auto err = ::getaddrinfo(nullptr, c_port.data(), &hints, &result);
			if (err != 0)
				return false;

			err = ::bind(m_handle, result->ai_addr, (int)result->ai_addrlen);
			if (err == -1)
			{
				::freeaddrinfo(result);
				return false;
			}

			::freeaddrinfo(result);
			return true;
		}

		bool listen(int max_connections) override
		{
			if (max_connections == 0)
				max_connections = SOMAXCONN;

			auto err = ::listen(m_handle, max_connections);
			if (err == -1)
				return false;

			return true;
		}

		Unique<Socket> accept() override
		{
			auto handle = ::accept(m_handle, nullptr, nullptr);
			if (handle == -1)
				return nullptr;

			return unique_from<LinuxSocket>(m_allocator, m_allocator, handle, m_family, m_type, m_protocol);
		}

		int64_t fd() override
		{
			return (int64_t)m_handle;
		}

		size_t read(void* buffer, size_t size) override
		{
			auto err = ::recv(m_handle, (char*)buffer, (int)size, 0);
			if (err == -1)
				return 0;

			return (size_t)err;
		}

		size_t write(const void* buffer, size_t size) override
		{
			auto err = ::send(m_handle, (const char*)buffer, (int)size, 0);
			if (err == -1)
				return 0;

			return (size_t)err;
		}

		int64_t seek(int64_t offset, SEEK_MODE whence) override
		{
			return -1;
		}

		int64_t tell() override
		{
			return -1;
		}
	};

	Unique<Socket> Socket::open(Allocator* allocator, FAMILY family, TYPE type)
	{
		int osFamily = 0;
		switch (family)
		{
		case FAMILY_UNSPEC:
			osFamily = AF_UNSPEC;
			break;
		case FAMILY_IPV4:
			osFamily = AF_INET;
			break;
		case FAMILY_IPV6:
			osFamily = AF_INET6;
			break;
		default:
			assert(false);
			break;
		}

		int osType = 0;
		int osProtocol = 0;
		switch (type)
		{
		case TYPE_TCP:
			osType = SOCK_STREAM;
			osProtocol = IPPROTO_TCP;
			break;
		case TYPE_UDP:
			osType = SOCK_DGRAM;
			osProtocol = IPPROTO_UDP;
			break;
		default:
			assert(false);
			break;
		}

		auto handle = ::socket(osFamily, osType, osProtocol);
		if (handle == -1)
			return nullptr;

		return unique_from<LinuxSocket>(allocator, allocator, handle, osFamily, osType, osProtocol);
	}
}