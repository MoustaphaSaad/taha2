#include "core/Rand.h"

#include <Windows.h>
#include <bcrypt.h>

namespace core
{
	bool Rand::cryptoRand(Span<std::byte> buffer)
	{
		auto res =
			BCryptGenRandom(nullptr, (PUCHAR)buffer.data(), (ULONG)buffer.count(), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		return BCRYPT_SUCCESS(res);
	}
}