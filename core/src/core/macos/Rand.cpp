#include "core/Rand.h"

#include <sys/random.h>

namespace core
{
	bool Rand::cryptoRand(Span<std::byte> buffer)
	{
		return getentropy(buffer.data(), buffer.count()) == 0;
	}
}