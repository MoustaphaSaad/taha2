#include "core/Msgpack.h"
#include "core/Intrinsics.h"

namespace core::msgpack
{
	HumanError Writer::write_blob(const void* data, size_t size)
	{
		auto res = m_stream->write(data, size);
		if (res < size)
			return errf(m_allocator, "failed to write {} bytes to stream, only {} bytes were written"_sv, size, res);
		return {};
	}

	HumanError Writer::write_uint8(uint8_t value)
	{
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_uint16(uint16_t value)
	{
		if (systemEndianness() == Endianness::Little)
			value = byteswap_uint16(value);
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_uint32(uint32_t value)
	{
		if (systemEndianness() == Endianness::Little)
			value = byteswap_uint32(value);
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_uint64(uint64_t value)
	{
		if (systemEndianness() == Endianness::Little)
			value = byteswap_uint64(value);
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_int8(int8_t value)
	{
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_int16(int16_t value)
	{
		if (systemEndianness() == Endianness::Little)
		{
			auto res = byteswap_uint16(*(uint16_t*)&value);
			value = *(int16_t*)&res;
		}
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_int32(int32_t value)
	{
		if (systemEndianness() == Endianness::Little)
		{
			auto res = byteswap_uint32(*(uint32_t*)&value);
			value = *(int32_t*)&res;
		}
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_int64(int64_t value)
	{
		if (systemEndianness() == Endianness::Little)
		{
			auto res = byteswap_uint64(*(uint64_t*)&value);
			value = *(int64_t*)&res;
		}
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_float32(float value)
	{
		if (systemEndianness() == Endianness::Little)
		{
			auto res = byteswap_uint32(*(uint32_t*)&value);
			value = *(float*)&res;
		}
		return write_blob(&value, sizeof(value));
	}

	HumanError Writer::write_float64(double value)
	{
		if (systemEndianness() == Endianness::Little)
		{
			auto res = byteswap_uint64(*(uint64_t*)&value);
			value = *(double*)&res;
		}
		return write_blob(&value, sizeof(value));
	}

	HumanError msgpack(Writer& writer, nullptr_t)
	{
		return writer.write_uint8(0xc0);
	}

	HumanError msgpack(Writer& writer, bool value)
	{
		return writer.write_uint8(value ? 0xc3 : 0xc2);
	}

	HumanError msgpack(Writer& writer, uint64_t value)
	{
		if (value <= 0x7f)
		{
			return writer.write_uint8((uint8_t)value);
		}
		else if (value <= UINT8_MAX)
		{
			if (auto err = writer.write_uint8(0xcc)) return err;
			return writer.write_uint8((uint8_t)value);
		}
		else if (value <= UINT16_MAX)
		{
			if (auto err = writer.write_uint8(0xcd)) return err;
			return writer.write_uint16((uint16_t)value);
		}
		else if (value <= UINT32_MAX)
		{
			if (auto err = writer.write_uint8(0xce)) return err;
			return writer.write_uint32((uint32_t)value);
		}
		else if (value <= UINT64_MAX)
		{
			if (auto err = writer.write_uint8(0xcf)) return err;
			return writer.write_uint64(value);
		}
		else
		{
			assert(false);
			return errf(writer.allocator(), "integer is too big"_sv);
		}
	}

	HumanError msgpack(Writer& writer, int64_t value)
	{
		if (value >= 0)
		{
			if (value <= 0x7f)
			{
				return writer.write_int8((int8_t)value);
			}
			else if (value <= INT8_MAX)
			{
				if (auto err = writer.write_uint8(0xd0)) return err;
				return writer.write_int8((int8_t)value);
			}
			else if (value <= INT16_MAX)
			{
				if (auto err = writer.write_uint8(0xd1)) return err;
				return writer.write_int16((int16_t)value);
			}
			else if (value <= INT32_MAX)
			{
				if (auto err = writer.write_uint8(0xd2)) return err;
				return writer.write_int32((int32_t)value);
			}
			else if (value <= INT64_MAX)
			{
				if (auto err = writer.write_uint8(0xd3)) return err;
				return writer.write_int64(value);
			}
			else
			{
				assert(false);
				return errf(writer.allocator(), "integer is too big"_sv);
			}
		}
		else
		{
			if (value >= -32)
			{
				return writer.write_int8((int8_t)value);
			}
			else if (value >= INT8_MIN)
			{
				if (auto err = writer.write_uint8(0xd0)) return err;
				return writer.write_int8((int8_t)value);
			}
			else if (value >= INT16_MIN)
			{
				if (auto err = writer.write_uint8(0xd1)) return err;
				return writer.write_int16((int16_t)value);
			}
			else if (value >= INT32_MIN)
			{
				if (auto err = writer.write_uint8(0xd2)) return err;
				return writer.write_int32((int32_t)value);
			}
			else if (value >= INT64_MIN)
			{
				if (auto err = writer.write_uint8(0xd3)) return err;
				return writer.write_int64(value);
			}
			else
			{
				assert(false);
				return errf(writer.allocator(), "integer is too small"_sv);
			}
		}
	}

	HumanError msgpack(Writer& writer, float value)
	{
		if (auto err = writer.write_uint8(0xca)) return err;
		return writer.write_float32(value);
	}

	HumanError msgpack(Writer& writer, double value)
	{
		if (auto err = writer.write_uint8(0xcb)) return err;
		return writer.write_float64(value);
	}

	HumanError msgpack(Writer& writer, StringView value)
	{
		if (value.count() <= 31)
		{
			auto prefix = (uint8_t)value.count();
			prefix |= 0xa0;
			if (auto err = writer.write_uint8(prefix)) return err;
			return writer.write_blob(value.data(), value.count());
		}
		else if (value.count() <= UINT8_MAX)
		{
			if (auto err = writer.write_uint8(0xd9)) return err;
			if (auto err = writer.write_uint8((uint8_t)value.count())) return err;
			return writer.write_blob(value.data(), value.count());
		}
		else if (value.count() <= UINT16_MAX)
		{
			if (auto err = writer.write_uint8(0xda)) return err;
			if (auto err = writer.write_uint16((uint16_t)value.count())) return err;
			return writer.write_blob(value.data(), value.count());
		}
		else if (value.count() <= UINT32_MAX)
		{
			if (auto err = writer.write_uint8(0xdb)) return err;
			if (auto err = writer.write_uint32((uint32_t)value.count())) return err;
			return writer.write_blob(value.data(), value.count());
		}
		else
		{
			assert(false);
			return errf(writer.allocator(), "string is too long"_sv);
		}
	}

	HumanError msgpack(Writer& writer, const void* data, size_t size)
	{
		if (size <= UINT8_MAX)
		{
			if (auto err = writer.write_uint8(0xc4)) return err;
			if (auto err = writer.write_uint8((uint8_t)size)) return err;
			return writer.write_blob(data, size);
		}
		else if (size <= UINT16_MAX)
		{
			if (auto err = writer.write_uint8(0xc5)) return err;
			if (auto err = writer.write_uint16((uint16_t)size)) return err;
			return writer.write_blob(data, size);
		}
		else if (size <= UINT32_MAX)
		{
			if (auto err = writer.write_uint8(0xc6)) return err;
			if (auto err = writer.write_uint32((uint32_t)size)) return err;
			return writer.write_blob(data, size);
		}
		else
		{
			assert(false);
			return errf(writer.allocator(), "blob is too big"_sv);
		}
	}

	HumanError Reader::read_blob(void* data, size_t size)
	{
		auto read_size = m_stream->read(data, size);
		if (size != read_size)
			return errf(m_allocator, "failed to read {} bytes, only {} was read"_sv, size, read_size);
		return {};
	}

	HumanError Reader::read_uint8(uint8_t& value)
	{
		return read_blob(&value, sizeof(value));
	}

	HumanError Reader::read_uint16(uint16_t& value)
	{
		if (auto err = read_blob(&value, sizeof(value))) return err;

		if (systemEndianness() == Endianness::Little)
			value = byteswap_uint16(value);

		return {};
	}

	HumanError Reader::read_uint32(uint32_t& value)
	{
		if (auto err = read_blob(&value, sizeof(value))) return err;

		if (systemEndianness() == Endianness::Little)
			value = byteswap_uint32(value);

		return {};
	}

	HumanError Reader::read_uint64(uint64_t& value)
	{
		if (auto err = read_blob(&value, sizeof(value))) return err;

		if (systemEndianness() == Endianness::Little)
			value = byteswap_uint64(value);

		return {};
	}

	HumanError Reader::read_int8(int8_t& value)
	{
		return read_blob(&value, sizeof(value));
	}

	HumanError Reader::read_int16(int16_t& value)
	{
		if (auto err = read_blob(&value, sizeof(value))) return err;

		if (systemEndianness() == Endianness::Little)
		{
			auto res = byteswap_uint16(*(uint16_t*)&value);
			value = *(int16_t*)&res;
		}

		return {};
	}

	HumanError Reader::read_int32(int32_t& value)
	{
		if (auto err = read_blob(&value, sizeof(value))) return err;

		if (systemEndianness() == Endianness::Little)
		{
			auto res = byteswap_uint32(*(uint32_t*)&value);
			value = *(int32_t*)&res;
		}

		return {};
	}

	HumanError Reader::read_int64(int64_t& value)
	{
		if (auto err = read_blob(&value, sizeof(value))) return err;

		if (systemEndianness() == Endianness::Little)
		{
			auto res = byteswap_uint64(*(uint64_t*)&value);
			value = *(int64_t*)&res;
		}

		return {};
	}

	HumanError Reader::read_float32(float& value)
	{
		if (auto err = read_blob(&value, sizeof(value))) return err;

		if (systemEndianness() == Endianness::Little)
		{
			void* p = &value;
			auto res = byteswap_uint32(*(uint32_t*)p);
			p = &res;
			value = *(float*)p;
		}

		return {};
	}

	HumanError Reader::read_float64(double& value)
	{
		if (auto err = read_blob(&value, sizeof(value))) return err;

		if (systemEndianness() == Endianness::Little)
		{
			void* p = &value;
			auto res = byteswap_uint64(*(uint64_t*)p);
			p = &res;
			value = *(double*)p;
		}

		return {};
	}

	Result<size_t> Reader::read_string_count(uint8_t prefix)
	{
		if (prefix >= 0xa0 && prefix <= 0xbf)
		{
			return prefix & 0x1f;
		}
		else if (prefix == 0xd9)
		{
			uint8_t count{};
			if (auto err = read_uint8(count)) return err;
			return count;
		}
		else if (prefix == 0xda)
		{
			uint16_t count{};
			if (auto err = read_uint16(count)) return err;
			return count;
		}
		else if (prefix == 0xdb)
		{
			uint32_t count{};
			if (auto err = read_uint32(count)) return err;
			return count;
		}
		else
		{
			assert(false);
			return errf(m_allocator, "invalid string prefix: {:x}"_sv, prefix);
		}
	}

	Result<size_t> Reader::read_array_count(uint8_t prefix)
	{
		if (prefix >= 0x90 && prefix <= 0x9f)
		{
			return prefix & 0x0f;
		}
		else if (prefix == 0xdc)
		{
			uint16_t count{};
			if (auto err = read_uint16(count)) return err;
			return count;
		}
		else if (prefix == 0xdd)
		{
			uint32_t count{};
			if (auto err = read_uint32(count)) return err;
			return count;
		}
		else
		{
			assert(false);
			return errf(m_allocator, "invalid array prefix: {:x}"_sv, prefix);
		}
	}

	Result<size_t> Reader::read_map_count(uint8_t prefix)
	{
		if (prefix >= 0x80 && prefix <= 0x8f)
		{
			return prefix & 0x0f;
		}
		else if (prefix == 0xde)
		{
			uint16_t count{};
			if (auto err = read_uint16(count)) return err;
			return count;
		}
		else if (prefix == 0xdf)
		{
			uint32_t count{};
			if (auto err = read_uint32(count)) return err;
			return count;
		}
		else
		{
			assert(false);
			return errf(m_allocator, "invalid map prefix: {:x}"_sv, prefix);
		}
	}

	Result<size_t> Reader::read_bin_count(uint8_t prefix)
	{
		if (prefix == 0xc4)
		{
			uint8_t count{};
			if (auto err = read_uint8(count)) return err;
			return count;
		}
		else if (prefix == 0xc5)
		{
			uint16_t count{};
			if (auto err = read_uint16(count)) return err;
			return count;
		}
		else if (prefix == 0xc6)
		{
			uint32_t count{};
			if (auto err = read_uint32(count)) return err;
			return count;
		}
		else
		{
			assert(false);
			return errf(m_allocator, "invalid bin prefix: {:x}"_sv, prefix);
		}
	}

	HumanError Reader::read_uint(uint8_t prefix, uint64_t& value)
	{
		if (prefix <= 0x7f)
		{
			value = prefix;
		}
		else if (prefix == 0xcc)
		{
			uint8_t v{};
			if (auto err = read_uint8(v)) return err;
			value = v;
		}
		else if (prefix == 0xcd)
		{
			uint16_t v{};
			if (auto err = read_uint16(v)) return err;
			value = v;
		}
		else if (prefix == 0xce)
		{
			uint32_t v{};
			if (auto err = read_uint32(v)) return err;
			value = v;
		}
		else if (prefix == 0xcf)
		{
			uint64_t v{};
			if (auto err = read_uint64(v)) return err;
			value = v;
		}
		else
		{
			int64_t v{};
			if (auto err = read_int(prefix, v)) return errf(m_allocator, "invalid uint prefix: {:x}"_sv, prefix);
			if (v < 0)
				return errf(m_allocator, "expected uint but found int '{}' instead"_sv, v);
			value = (uint64_t)v;
		}
		return {};
	}

	HumanError Reader::read_int(uint8_t prefix, int64_t& value)
	{
		if (prefix <= 0x7f || prefix >= 0xe0)
		{
			value = *(int8_t*)&prefix;
		}
		else if (prefix == 0xd0)
		{
			int8_t v{};
			if (auto err = read_int8(v)) return err;
			value = v;
		}
		else if (prefix == 0xd1)
		{
			int16_t v{};
			if (auto err = read_int16(v)) return err;
			value = v;
		}
		else if (prefix == 0xd2)
		{
			int32_t v{};
			if (auto err = read_int32(v)) return err;
			value = v;
		}
		else if (prefix == 0xd3)
		{
			int64_t v{};
			if (auto err = read_int64(v)) return err;
			value = v;
		}
		else
		{
			uint64_t v{};
			if (auto err = read_uint(prefix, v)) return errf(m_allocator, "invalid int prefix: {:x}"_sv, prefix);
			if (v > INT64_MAX)
				return errf(m_allocator, "expected int but found uint '{}' instead"_sv, v);
			value = (int64_t)v;
		}
		return {};
	}

	HumanError Reader::skip()
	{
		uint8_t prefix{};
		if (auto err = read_uint8(prefix)) return err;

		if (prefix <= 0x7f || (prefix >= 0xcc && prefix <= 0xcf))
		{
			uint64_t value{};
			if (auto err = read_uint(prefix, value)) return err;
		}
		else if (prefix >= 0xe0 || (prefix >= 0xd0 && prefix <= 0xd3))
		{
			int64_t value{};
			if (auto err = read_int(prefix, value)) return err;
		}
		else if (prefix == 0xca)
		{
			float value{};
			if (auto err = read_float32(value)) return err;
		}
		else if (prefix == 0xcb)
		{
			double value{};
			if (auto err = read_float64(value)) return err;
		}
		else if (prefix == 0xc2 || prefix == 0xc3)
		{
			// bool is already encoded in the prefix
		}
		else if ((prefix >= 0xa0 && prefix <= 0xbf) || (prefix >= 0xd9 && prefix <= 0xdb))
		{
			String value{m_allocator};

			auto count = read_string_count(prefix);
			if (count.is_error()) return count.release_error();

			value.resize(count.value());
			if (auto err = read_blob(value.data(), value.count())) return err;
		}
		else if (prefix >= 0xc4 && prefix <= 0xc6)
		{
			Buffer value{m_allocator};

			auto count = read_bin_count(prefix);
			if (count.is_error()) return count.release_error();

			value.resize(count.value());
			if (auto err = read_blob(value.data(), value.count())) return err;
		}
		else if (prefix >= 0x90 && prefix <= 0x9f)
		{
			auto count = prefix & 0xf;
			for (int i = 0; i < count; ++i)
				if (auto err = skip()) return err;
		}
		else if (prefix == 0xdc)
		{
			uint16_t count{};
			if (auto err = read_uint16(count)) return err;

			for (uint16_t i = 0; i < count; ++i)
				if (auto err = skip()) return err;
		}
		else if (prefix == 0xdd)
		{
			uint32_t count{};
			if (auto err = read_uint32(count)) return err;

			for (uint32_t i = 0; i < count; ++i)
				if (auto err = skip()) return err;
		}
		else if (prefix >= 0x80 && prefix <= 0x8f)
		{
			auto count = prefix & 0xf;
			for (int i = 0; i < count; ++i)
			{
				// name
				if (auto err = skip()) return err;
				// value
				if (auto err = skip()) return err;
			}
		}
		else if (prefix == 0xde)
		{
			uint16_t count{};
			if (auto err = read_uint16(count)) return err;

			for (uint16_t i = 0; i < count; ++i)
			{
				// name
				if (auto err = skip()) return err;
				// value
				if (auto err = skip()) return err;
			}
		}
		else if (prefix == 0xdf)
		{
			uint32_t count{};
			if (auto err = read_uint32(count)) return err;

			for (uint32_t i = 0; i < count; ++i)
			{
				// name
				if (auto err = skip()) return err;
				// value
				if (auto err = skip()) return err;
			}
		}
		else
		{
			assert(false);
			return errf(m_allocator, "invalid prefix: {:x}"_sv, prefix);
		}

		return {};
	}

	HumanError msgpack(Reader& reader, bool& value)
	{
		uint8_t rep{};
		if (auto err = reader.read_uint8(rep)) return err;
		if (rep == 0xc3)
			value = true;
		else if (rep == 0xc2)
			value = false;
		else
			return errf(reader.allocator(), "invalid bool representation: {:x}"_sv, rep);
		return {};
	}

	HumanError msgpack(Reader& reader, uint64_t& value)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;
		return reader.read_uint(prefix, value);
	}

	HumanError msgpack(Reader& reader, uint8_t& value)
	{
		uint64_t v{};
		if (auto err = msgpack(reader, v)) return err;
		if (v > UINT8_MAX)
			return errf(reader.allocator(), "expected uint8 but found uint64 '{}'"_sv, v);
		value = (uint8_t)v;
		return {};
	}

	HumanError msgpack(Reader& reader, uint16_t& value)
	{
		uint64_t v{};
		if (auto err = msgpack(reader, v)) return err;
		if (v > UINT16_MAX)
			return errf(reader.allocator(), "expected uint16 but found uint64 '{}'"_sv, v);
		value = (uint16_t)v;
		return {};
	}

	HumanError msgpack(Reader& reader, uint32_t& value)
	{
		uint64_t v{};
		if (auto err = msgpack(reader, v)) return err;
		if (v > UINT32_MAX)
			return errf(reader.allocator(), "expected uint32 but found uint64 '{}'"_sv, v);
		value = (uint32_t)v;
		return {};
	}

	HumanError msgpack(Reader& reader, int64_t& value)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;
		return reader.read_int(prefix, value);
	}

	HumanError msgpack(Reader& reader, int8_t& value)
	{
		int64_t v{};
		if (auto err = msgpack(reader, v)) return err;
		if (v > INT8_MAX)
			return errf(reader.allocator(), "int8 overflow, value is '{}'"_sv, v);
		else if (v < INT8_MIN)
			return errf(reader.allocator(), "int8 underflow, value is '{}'"_sv, v);
		value = (int8_t)v;
		return {};
	}

	HumanError msgpack(Reader& reader, int16_t& value)
	{
		int64_t v{};
		if (auto err = msgpack(reader, v)) return err;
		if (v > INT16_MAX)
			return errf(reader.allocator(), "int16 overflow, value is '{}'"_sv, v);
		else if (v < INT16_MIN)
			return errf(reader.allocator(), "int16 underflow, value is '{}'"_sv, v);
		value = (int16_t)v;
		return {};
	}

	HumanError msgpack(Reader& reader, int32_t& value)
	{
		int64_t v{};
		if (auto err = msgpack(reader, v)) return err;
		if (v > INT32_MAX)
			return errf(reader.allocator(), "int32 overflow, value is '{}'"_sv, v);
		else if (v < INT32_MIN)
			return errf(reader.allocator(), "int32 underflow, value is '{}'"_sv, v);
		value = (int32_t)v;
		return {};
	}

	HumanError msgpack(Reader& reader, float& value)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;

		if (prefix != 0xca)
			return errf(reader.allocator(), "invalid float prefix: {:x}"_sv, prefix);

		float v{};
		if (auto err = reader.read_float32(v)) return err;
		value = v;
		return {};
	}

	HumanError msgpack(Reader& reader, double& value)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;

		if (prefix != 0xcb)
			return errf(reader.allocator(), "invalid double prefix: {:x}"_sv, prefix);

		double v{};
		if (auto err = reader.read_float64(v)) return err;
		value = v;
		return {};
	}

	HumanError msgpack(Reader& reader, String& value)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;

		auto count = reader.read_string_count(prefix);
		if (count.is_error())
			return count.release_error();

		value.resize(count.value());
		if (auto err = reader.read_blob(value.data(), value.count())) return err;
		return {};
	}

	HumanError msgpack(Reader& reader, Buffer& value)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;

		auto count = reader.read_bin_count(prefix);
		if (count.is_error())
			return count.release_error();

		value.resize(count.value());
		if (auto err = reader.read_blob(value.data(), value.count())) return err;
		return {};
	}
}