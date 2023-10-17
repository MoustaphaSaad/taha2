#pragma once

#include "core/Exports.h"
#include "core/Stream.h"
#include "core/Unique.h"
#include "core/Allocator.h"

namespace core
{
	class Socket: public Stream
	{
	public:
		enum FAMILY
		{
			FAMILY_UNSPEC,
			FAMILY_IPV4,
			FAMILY_IPV6,
		};

		enum TYPE
		{
			TYPE_TCP,
			TYPE_UDP,
		};

		CORE_EXPORT static Unique<Socket> open(Allocator* allocator, FAMILY family, TYPE type);

		virtual bool close() = 0;
		virtual bool connect(StringView address, StringView port) = 0;
		virtual bool bind(StringView port) = 0;
		virtual bool listen(int max_connections = 0) = 0;
		virtual Unique<Socket> accept() = 0;
		virtual int64_t fd() = 0;
	};
}