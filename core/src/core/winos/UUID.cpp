#include "core/UUID.h"

#include <Windows.h>
#include <bcrypt.h>
#include <cassert>

namespace core
{
	inline bool crypto_rand(void* buffer, size_t size)
	{
		auto res = BCryptGenRandom(nullptr, (PUCHAR)buffer, (ULONG)size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		return BCRYPT_SUCCESS(res);
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