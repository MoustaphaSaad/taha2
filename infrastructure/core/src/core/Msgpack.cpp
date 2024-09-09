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
			coreUnreachable();
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
				coreUnreachable();
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
				coreUnreachable();
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
			coreUnreachable();
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
			coreUnreachable();
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
			coreUnreachable();
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
			coreUnreachable();
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
			coreUnreachable();
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
			coreUnreachable();
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
			if (count.isError()) return count.releaseError();

			value.resize(count.value());
			if (auto err = read_blob(value.data(), value.count())) return err;
		}
		else if (prefix >= 0xc4 && prefix <= 0xc6)
		{
			Buffer value{m_allocator};

			auto count = read_bin_count(prefix);
			if (count.isError()) return count.releaseError();

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
			coreUnreachable();
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
		if (count.isError())
			return count.releaseError();

		value.resize(count.value());
		if (auto err = reader.read_blob(value.data(), value.count())) return err;
		return {};
	}

	HumanError msgpack(Reader& reader, Buffer& value)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;

		auto count = reader.read_bin_count(prefix);
		if (count.isError())
			return count.releaseError();

		value.resize(count.value());
		if (auto err = reader.read_blob(value.data(), value.count())) return err;
		return {};
	}

	Value::Value(StringView value, Allocator* allocator) : m_kind{KIND_STRING}
	{
		m_string = allocator->allocSingleT<String>();
		allocator->commitSingleT(m_string);
		new (m_string) String{value, allocator};
	}

	Value::Value(const std::byte* data, size_t size, Allocator* allocator) : m_kind{KIND_BYTES}
	{
		m_bytes = allocator->allocSingleT<Buffer>();
		allocator->commitSingleT(m_bytes);
		new (m_bytes) Buffer{allocator};
		m_bytes->push(data, size);
	}

	Value::Value(Array<Value> value) : m_kind{KIND_ARRAY}
	{
		auto allocator = value.allocator();
		m_array = allocator->allocSingleT<Array<Value>>();
		allocator->commitSingleT(m_array);
		new (m_array) Array<Value>{std::move(value)};
	}

	Value::Value(Map<String, Value> value) : m_kind{KIND_MAP}
	{
		auto allocator = value.allocator();
		m_map = allocator->allocSingleT<Map<String, Value>>();
		allocator->commitSingleT(m_map);
		new (m_map) Map<String, Value>{std::move(value)};
	}

	Value& Value::operator=(nullptr_t)
	{
		destroy();
		m_kind = KIND_NULL;
		return *this;
	}

	Value& Value::operator=(bool value)
	{
		destroy();
		m_kind = KIND_BOOL;
		m_bool = value;
		return *this;
	}

	Value& Value::operator=(int64_t value)
	{
		destroy();
		m_kind = KIND_INT;
		m_int = value;
		return *this;
	}

	Value& Value::operator=(uint64_t value)
	{
		destroy();
		m_kind = KIND_UINT;
		m_uint = value;
		return *this;
	}

	Value& Value::operator=(float value)
	{
		destroy();
		m_kind = KIND_FLOAT;
		m_float = value;
		return *this;
	}

	Value& Value::operator=(double value)
	{
		destroy();
		m_kind = KIND_DOUBLE;
		m_double = value;
		return *this;
	}

	Value& Value::operator=(String value)
	{
		destroy();
		m_kind = KIND_STRING;
		auto allocator = value.allocator();
		m_string = allocator->allocSingleT<String>();
		allocator->commitSingleT(m_string);
		new (m_string) String{std::move(value)};
		return *this;
	}

	Value& Value::operator=(Buffer value)
	{
		destroy();
		m_kind = KIND_BYTES;
		auto allocator = value.allocator();
		m_bytes = allocator->allocSingleT<Buffer>();
		allocator->commitSingleT(m_bytes);
		new (m_bytes) Buffer{std::move(value)};
		return *this;
	}

	Value& Value::operator=(Array<Value> value)
	{
		destroy();
		m_kind = KIND_ARRAY;
		auto allocator = value.allocator();
		m_array = allocator->allocSingleT<Array<Value>>();
		allocator->commitSingleT(m_array);
		new (m_array) Array<Value>{std::move(value)};
		return *this;
	}

	Value& Value::operator=(Map<String, Value> value)
	{
		destroy();
		m_kind = KIND_MAP;
		auto allocator = value.allocator();
		m_map = allocator->allocSingleT<Map<String, Value>>();
		allocator->commitSingleT(m_map);
		new (m_map) Map<String, Value>{std::move(value)};
		return *this;
	}

	String Value::release_string()
	{
		coreAssert(m_kind == KIND_STRING);
		auto allocator = m_string->allocator();
		auto res = std::move(*m_string);
		allocator->releaseSingleT(m_string);
		allocator->freeSingleT(m_string);
		return res;
	}

	Buffer Value::release_bytes()
	{
		coreAssert(m_kind == KIND_BYTES);
		auto allocator = m_bytes->allocator();
		auto res = std::move(*m_bytes);
		allocator->releaseSingleT(m_bytes);
		allocator->freeSingleT(m_bytes);
		return res;
	}

	Array<Value> Value::release_array()
	{
		coreAssert(m_kind == KIND_ARRAY);
		auto allocator = m_array->allocator();
		auto res = std::move(*m_array);
		allocator->releaseSingleT(m_array);
		allocator->freeSingleT(m_array);
		return res;
	}

	Map<String, Value> Value::release_map()
	{
		coreAssert(m_kind == KIND_MAP);
		auto allocator = m_map->allocator();
		auto res = std::move(*m_map);
		allocator->releaseSingleT(m_map);
		allocator->freeSingleT(m_map);
		return res;
	}

	void Value::destroy()
	{
		switch (m_kind)
		{
		case KIND_STRING:
		{
			auto allocator = m_string->allocator();
			m_string->~String();
			allocator->releaseSingleT(m_string);
			allocator->freeSingleT(m_string);
			break;
		}
		case KIND_BYTES:
		{
			auto allocator = m_bytes->allocator();
			m_bytes->~Buffer();
			allocator->releaseSingleT(m_bytes);
			allocator->freeSingleT(m_bytes);
			break;
		}
		case KIND_ARRAY:
		{
			auto allocator = m_array->allocator();
			m_array->~Array<Value>();
			allocator->releaseSingleT(m_array);
			allocator->freeSingleT(m_array);
			break;
		}
		case KIND_MAP:
		{
			auto allocator = m_map->allocator();
			m_map->~Map<String, Value>();
			allocator->releaseSingleT(m_map);
			allocator->freeSingleT(m_map);
			break;
		}
		default:
			coreUnreachable();
			break;
		}
		m_kind = KIND_NULL;
	}

	void Value::copyFrom(const Value& other)
	{
		m_kind = other.m_kind;
		switch (m_kind)
		{
		case KIND_NULL:
			break;
		case KIND_BOOL:
			m_bool = other.m_bool;
			break;
		case KIND_INT:
			m_int = other.m_int;
			break;
		case KIND_UINT:
			m_uint = other.m_uint;
			break;
		case KIND_FLOAT:
			m_float = other.m_float;
			break;
		case KIND_DOUBLE:
			m_double = other.m_double;
			break;
		case KIND_STRING:
		{
			auto allocator = other.m_string->allocator();
			m_string = allocator->allocSingleT<String>();
			allocator->commitSingleT(m_string);
			new (m_string) String{*other.m_string};
			break;
		}
		case KIND_BYTES:
		{
			auto allocator = other.m_bytes->allocator();
			m_bytes = allocator->allocSingleT<Buffer>();
			allocator->commitSingleT(m_bytes);
			new (m_bytes) Buffer{*other.m_bytes};
			break;
		}
		case KIND_ARRAY:
		{
			auto allocator = other.m_array->allocator();
			m_array = allocator->allocSingleT<Array<Value>>();
			allocator->commitSingleT(m_array);
			new (m_array) Array<Value>{*other.m_array};
			break;
		}
		case KIND_MAP:
		{
			auto allocator = other.m_map->allocator();
			m_map = allocator->allocSingleT<Map<String, Value>>();
			allocator->commitSingleT(m_map);
			new (m_map) Map<String, Value>{*other.m_map};
			break;
		}
		default:
			coreUnreachable();
			break;
		}
	}

	void Value::moveFrom(Value& other)
	{
		m_kind = other.m_kind;
		switch (m_kind)
		{
		case KIND_NULL:
			break;
		case KIND_BOOL:
			m_bool = other.m_bool;
			break;
		case KIND_INT:
			m_int = other.m_int;
			break;
		case KIND_UINT:
			m_uint = other.m_uint;
			break;
		case KIND_FLOAT:
			m_float = other.m_float;
			break;
		case KIND_DOUBLE:
			m_double = other.m_double;
			break;
		case KIND_STRING:
			m_string = other.m_string;
			other.m_string = nullptr;
			break;
		case KIND_BYTES:
			m_bytes = other.m_bytes;
			other.m_bytes = nullptr;
			break;
		case KIND_ARRAY:
			m_array = other.m_array;
			other.m_array = nullptr;
			break;
		case KIND_MAP:
			m_map = other.m_map;
			other.m_map = nullptr;
			break;
		default:
			coreUnreachable();
			break;
		}
		other.m_kind = KIND_NULL;
	}

	HumanError msgpack(Writer& writer, const Value& value)
	{
		switch (value.kind())
		{
		case Value::KIND_NULL:
			return msgpack(writer, nullptr);
		case Value::KIND_BOOL:
			return msgpack(writer, value.as_bool());
		case Value::KIND_INT:
			return msgpack(writer, value.as_int());
		case Value::KIND_UINT:
			return msgpack(writer, value.as_uint());
		case Value::KIND_FLOAT:
			return msgpack(writer, value.as_float());
		case Value::KIND_DOUBLE:
			return msgpack(writer, value.as_double());
		case Value::KIND_STRING:
			return msgpack(writer, value.as_string());
		case Value::KIND_BYTES:
			return msgpack(writer, value.as_bytes());
		case Value::KIND_ARRAY:
			return msgpack(writer, value.as_array());
		case Value::KIND_MAP:
			return msgpack(writer, value.as_map());
		default:
			coreUnreachable();
			return errf(writer.allocator(), "invalid value kind: {}"_sv, (int)value.kind());
		}
	}

	HumanError msgpack(Reader& reader, Value& value)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;

		if (prefix <= 0x7f)
		{
			// positive fixint
			value = prefix;
		}
		else if (prefix >= 0xe0)
		{
			// negative fixint
			value = *(int8_t*)&prefix;
		}
		else if (prefix == 0xcc)
		{
			// uint8
			uint8_t v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xcd)
		{
			// uint16
			uint16_t v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xce)
		{
			// uint32
			uint32_t v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xcf)
		{
			// uint64
			uint64_t v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xd0)
		{
			// int8
			int8_t v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xd1)
		{
			// int16
			int16_t v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xd2)
		{
			// int32
			int32_t v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xd3)
		{
			// int64
			int64_t v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xca)
		{
			// float
			float v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xcb)
		{
			// double
			double v{};
			if (auto err = msgpack(reader, v)) return err;
			value = v;
		}
		else if (prefix == 0xc2)
		{
			// bool: false
			value = false;
		}
		else if (prefix == 0xc3)
		{
			// bool: true
			value = true;
		}
		else if (prefix == 0xc0)
		{
			// null
			value = nullptr;
		}
		else if ((prefix >= 0xa0 && prefix <= 0xbf) || prefix == 0xd9 || prefix == 0xda || prefix == 0xdb)
		{
			String v{reader.allocator()};
			if (auto err = msgpack(reader, v)) return err;
			value = std::move(v);
		}
		else if (prefix == 0xc4 || prefix == 0xc5 || prefix == 0xc6)
		{
			Buffer v{reader.allocator()};
			if (auto err = msgpack(reader, v)) return err;
			value = std::move(v);
		}
		else if ((prefix >= 0x90 && prefix <= 0x9f) || prefix == 0xdc || prefix == 0xdd)
		{
			Array<Value> v{reader.allocator()};
			if (auto err = msgpack(reader, v)) return err;
			value = std::move(v);
		}
		else if ((prefix >= 0x80 && prefix <= 0x8f) || prefix == 0xde || prefix == 0xdf)
		{
			Map<String, Value> v{reader.allocator()};
			if (auto err = msgpack(reader, v)) return err;
			value = std::move(v);
		}
		else
		{
			coreUnreachable();
			return errf(reader.allocator(), "invalid prefix: {:x}"_sv, prefix);
		}

		return {};
	}
}