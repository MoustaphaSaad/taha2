#include "core/Rand.h"

#include <sys/random.h>

namespace core
{
	bool Rand::cryptoRand(Span<std::byte> buffer)
	{
		size_t s = 0;
		while (s < buffer.count())
		{
			s += getrandom((char*)buffer.data() + s, buffer.count() - s, 0);
		}
		return true;
	}
}