#include "core/SHA1.h"

#include <openssl/sha.h>

namespace core
{
	SHA1 SHA1::hash(Span<std::byte> bytes)
	{
		SHA_CTX ctx;
		SHA1_Init(&ctx);
		SHA1_Update(&ctx, bytes.data(), bytes.count());

		SHA1 sha{};
		SHA1_Final((unsigned char*)sha.m_digest, &ctx);

		return sha;
	}
}