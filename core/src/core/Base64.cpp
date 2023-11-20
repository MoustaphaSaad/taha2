#include "core/Base64.h"

#include <openssl/bio.h>
#include <openssl/evp.h>

namespace core
{
	String Base64::encode(Span<std::byte> bytes, core::Allocator *allocator)
	{
		auto b64 = BIO_new(BIO_f_base64());
		// ignore newlines
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

		auto mem = BIO_new(BIO_s_mem());

		b64 = BIO_push(b64, mem);
		auto res = BIO_write(b64, bytes.data(), int(bytes.count()));
		assert(res == bytes.count());
		BIO_flush(b64);

		char* encodedString = nullptr;
		auto len = BIO_get_mem_data(mem, &encodedString);
		auto result = String{StringView{encodedString, size_t(len)}, allocator};
		BIO_free(b64);
		BIO_free(mem);

		return result;
	}

	Buffer Base64::decode(core::StringView str, core::Allocator *allocator)
	{
		auto b64 = BIO_new(BIO_f_base64());
		// ignore newlines
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

		auto mem = BIO_new_mem_buf(str.begin(), int(str.count()));

		b64 = BIO_push(b64, mem);

		auto decoded = Buffer{allocator};
		size_t maxlen = str.count() / 4 * 3 + 1;
		decoded.resize(maxlen);
		auto len = BIO_read(b64, decoded.data(), int(decoded.count()));
		decoded.resize(len);
		BIO_free(b64);
		BIO_free(mem);

		return decoded;
	}
}