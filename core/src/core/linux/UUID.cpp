#include "core/UUID.h"

#include <sys/random.h>
#include <cassert>

namespace core
{
	inline bool crypto_rand(void* buffer, size_t size)
	{
		size_t s = 0;
		while (s < size)
		{
			s += getrandom(buffer, size - s, 0);
		}
	}

	UUID UUID::generate()
	{
		UUID uuid;
		auto ok = crypto_rand(&uuid.data, sizeof(uuid.data));
		assert(ok);
		// version 4
		uuid.data.bytes[6] = (uuid.data.bytes[6] & 0x0f) | 0x40;
		// variant is 10
		uuid.data.bytes[8] = (uuid.data.bytes[8] & 0x3f) | 0x80;
		return uuid;
	}
}