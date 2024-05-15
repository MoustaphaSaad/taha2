#pragma once

#include "core/Exports.h"
#include "core/Stream.h"
#include "core/Allocator.h"
#include "core/Result.h"
#include "core/String.h"
#include "core/Buffer.h"
#include "core/Array.h"
#include "core/Hash.h"
#include "core/Assert.h"

namespace core::msgpack
{
	class Writer
	{
		Stream* m_stream = nullptr;
		Allocator* m_allocator = nullptr;
	public:
		Writer(Stream* stream, Allocator* allocator)
			: m_stream(stream),
			  m_allocator(allocator)
		{}

		Allocator* allocator() const { return m_allocator; }

		CORE_EXPORT HumanError write_blob(const void* data, size_t size);
		CORE_EXPORT HumanError write_uint8(uint8_t value);
		CORE_EXPORT HumanError write_uint16(uint16_t value);
		CORE_EXPORT HumanError write_uint32(uint32_t value);
		CORE_EXPORT HumanError write_uint64(uint64_t value);
		CORE_EXPORT HumanError write_int8(int8_t value);
		CORE_EXPORT HumanError write_int16(int16_t value);
		CORE_EXPORT HumanError write_int32(int32_t value);
		CORE_EXPORT HumanError write_int64(int64_t value);
		CORE_EXPORT HumanError write_float32(float value);
		CORE_EXPORT HumanError write_float64(double value);
	};

	CORE_EXPORT HumanError msgpack(Writer& writer, nullptr_t);
	CORE_EXPORT HumanError msgpack(Writer& writer, bool value);
	CORE_EXPORT HumanError msgpack(Writer& writer, uint64_t value);
	CORE_EXPORT HumanError msgpack(Writer& writer, int64_t value);
	CORE_EXPORT HumanError msgpack(Writer& writer, float value);
	CORE_EXPORT HumanError msgpack(Writer& writer, double value);
	CORE_EXPORT HumanError msgpack(Writer& writer, StringView value);
	CORE_EXPORT HumanError msgpack(Writer& writer, const void* data, size_t size);

	template<typename T>
	inline HumanError msgpack(Writer& writer, const Array<T>& value)
	{
		if (value.count() <= 15)
		{
			auto prefix = (uint8_t)value.count();
			prefix |= 0x90;
			if (auto err = writer.write_uint8(prefix)) return err;
		}
		else if (value.count() <= UINT16_MAX)
		{
			if (auto err = writer.write_uint8(0xdc)) return err;
			if (auto err = writer.write_uint16((uint16_t)value.count())) return err;
		}
		else if (value.count() <= UINT32_MAX)
		{
			if (auto err = writer.write_uint8(0xdd)) return err;
			if (auto err = writer.write_uint32((uint32_t)value.count())) return err;
		}
		else
		{
			coreUnreachable();
			return errf(writer.allocator(), "array is too long"_sv);
		}

		for (const auto& element: value)
			if (auto err = msgpack(writer, element))
				return err;
		return {};
	}

	template<typename T, size_t N>
	inline HumanError msgpack(Writer& writer, const T (&arr)[N])
	{
		if (N <= 15)
		{
			auto prefix = (uint8_t)N;
			prefix |= 0x90;
			if (auto err = writer.write_uint8(prefix)) return err;
		}
		else if (N <= UINT16_MAX)
		{
			if (auto err = writer.write_uint8(0xdc)) return err;
			if (auto err = writer.write_uint16((uint16_t)N)) return err;
		}
		else if (N <= UINT32_MAX)
		{
			if (auto err = writer.write_uint8(0xdd)) return err;
			if (auto err = writer.write_uint32((uint32_t)N)) return err;
		}
		else
		{
			coreUnreachable();
			return errf(writer.allocator(), "array is too long"_sv);
		}

		for (const auto& element: arr)
			if (auto err = msgpack(writer, element))
				return err;
		return {};
	}

	template<typename TKey, typename TValue, typename THash>
	inline HumanError msgpack(Writer& writer, const Map<TKey, TValue, THash>& value)
	{
		if (value.count() <= 15)
		{
			auto prefix = (uint8_t)value.count();
			prefix |= 0x80;
			if (auto err = writer.write_uint8(prefix)) return err;
		}
		else if (value.count() <= UINT16_MAX)
		{
			if (auto err = writer.write_uint8(0xde)) return err;
			if (auto err = writer.write_uint16((uint16_t)value.count())) return err;
		}
		else if (value.count() <= UINT32_MAX)
		{
			if (auto err = writer.write_uint8(0xdf)) return err;
			if (auto err = writer.write_uint32((uint32_t)value.count())) return err;
		}
		else
		{
			coreUnreachable();
			return errf(writer.allocator(), "map is too long"_sv);
		}

		for (const auto& element: value)
		{
			if (auto err = msgpack(writer, element.key)) return err;
			if (auto err = msgpack(writer, element.value)) return err;
		}
		return {};
	}

	inline HumanError msgpack(Writer& writer, uint8_t value) { return msgpack(writer, (uint64_t)value); }
	inline HumanError msgpack(Writer& writer, uint16_t value) { return msgpack(writer, (uint64_t)value); }
	inline HumanError msgpack(Writer& writer, uint32_t value) { return msgpack(writer, (uint64_t)value); }
	inline HumanError msgpack(Writer& writer, int8_t value) { return msgpack(writer, (int64_t)value); }
	inline HumanError msgpack(Writer& writer, int16_t value) { return msgpack(writer, (int64_t)value); }
	inline HumanError msgpack(Writer& writer, int32_t value) { return msgpack(writer, (int64_t)value); }
	inline HumanError msgpack(Writer& writer, const String& value) { return msgpack(writer, (StringView)value); }
	inline HumanError msgpack(Writer& writer, const Buffer& value) { return msgpack(writer, value.data(), value.count()); }

	class Reader
	{
		Stream* m_stream = nullptr;
		Allocator* m_allocator = nullptr;
	public:
		Reader(Stream* stream, Allocator* allocator)
			: m_stream(stream),
			  m_allocator(allocator)
		{}

		Allocator* allocator() const { return m_allocator; }

		CORE_EXPORT HumanError read_blob(void* data, size_t size);
		CORE_EXPORT HumanError read_uint8(uint8_t& value);
		CORE_EXPORT HumanError read_uint16(uint16_t& value);
		CORE_EXPORT HumanError read_uint32(uint32_t& value);
		CORE_EXPORT HumanError read_uint64(uint64_t& value);
		CORE_EXPORT HumanError read_int8(int8_t& value);
		CORE_EXPORT HumanError read_int16(int16_t& value);
		CORE_EXPORT HumanError read_int32(int32_t& value);
		CORE_EXPORT HumanError read_int64(int64_t& value);
		CORE_EXPORT HumanError read_float32(float& value);
		CORE_EXPORT HumanError read_float64(double& value);

		CORE_EXPORT Result<size_t> read_string_count(uint8_t prefix);
		CORE_EXPORT Result<size_t> read_array_count(uint8_t prefix);
		CORE_EXPORT Result<size_t> read_map_count(uint8_t prefix);
		CORE_EXPORT Result<size_t> read_bin_count(uint8_t prefix);

		CORE_EXPORT HumanError read_uint(uint8_t prefix, uint64_t& value);
		CORE_EXPORT HumanError read_int(uint8_t prefix, int64_t& value);

		CORE_EXPORT HumanError skip();
	};

	CORE_EXPORT HumanError msgpack(Reader& reader, bool& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, uint64_t& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, uint8_t& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, uint16_t& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, uint32_t& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, int64_t& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, int8_t& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, int16_t& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, int32_t& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, float& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, double& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, String& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, Buffer& value);

	template<typename T>
	struct DefaultConstruct
	{
		T operator()(Reader& reader) const { return T{}; }
	};

	template<>
	struct DefaultConstruct<String>
	{
		String operator()(Reader& reader) const { return String{reader.allocator()}; }
	};

	template<>
	struct DefaultConstruct<Buffer>
	{
		Buffer operator()(Reader& reader) const { return Buffer{reader.allocator()}; }
	};

	template<typename T>
	struct DefaultConstruct<Array<T>>
	{
		Array<T> operator()(Reader& reader) const { return Array<T>{reader.allocator()}; }
	};

	template<typename TKey, typename TValue, typename THash>
	struct DefaultConstruct<Map<TKey, TValue, THash>>
	{
		Map<TKey, TValue, THash> operator()(Reader& reader) const { return Map<TKey, TValue, THash>{reader.allocator()}; }
	};

	template<typename T>
	inline HumanError msgpack(Reader& reader, Array<T>& value)
	{
		uint8_t prefix;
		if (auto err = reader.read_uint8(prefix)) return err;

		auto count = reader.read_array_count(prefix);
		if (count.isError()) return count.releaseError();

		value.clear();
		value.reserve(count.value());
		for (size_t i = 0; i < count.value(); ++i)
		{
			auto element = DefaultConstruct<T>{}(reader);
			if (auto err = msgpack(reader, element))
				return err;
			value.push(std::move(element));
		}
		return {};
	}

	template<typename T, size_t N>
	inline HumanError msgpack(Reader& reader, T (&value)[N])
	{
		uint8_t prefix;
		if (auto err = reader.read_uint8(prefix)) return err;

		auto count = reader.read_array_count(prefix);
		if (count.isError()) return count.releaseError();

		if (count.value() != N)
			return errf(reader.allocator(), "array size mismatch"_sv);

		for (size_t i = 0; i < N; ++i)
		{
			if (auto err = msgpack(reader, value[i]))
				return err;
		}
		return {};
	}

	template<typename TKey, typename TValue, typename THash>
	inline HumanError msgpack(Reader& reader, Map<TKey, TValue, THash>& res)
	{
		uint8_t prefix;
		if (auto err = reader.read_uint8(prefix)) return err;

		auto count = reader.read_map_count(prefix);
		if (count.isError()) return count.releaseError();

		res.clear();
		for (size_t i = 0; i < count.value(); ++i)
		{
			auto key = DefaultConstruct<TKey>{}(reader);
			if (auto err = msgpack(reader, key)) return err;

			auto value = DefaultConstruct<TValue>{}(reader);
			if (auto err = msgpack(reader, value)) return err;

			res.insert(std::move(key), std::move(value));
		}
		return {};
	}

	class Field
	{
		StringView m_name = ""_sv;
		void* m_value = nullptr;
		HumanError (*m_write)(Writer&, const void*) = nullptr;
		HumanError (*m_read)(Reader&, void*) = nullptr;
	public:
		template<typename T>
		Field(StringView name, T& value)
			: m_name{name},
			  m_value{(void*)&value}
		{
			m_write = [](Writer& writer, const void* value) -> HumanError {
				return msgpack(writer, *(const T*)value);
			};
			m_read = [](Reader& reader, void* value) -> HumanError {
				return msgpack(reader, *(T*)value);
			};
		}

		StringView name() const { return m_name; }
		HumanError write(Writer& writer) const { return m_write(writer, m_value); }
		HumanError read(Reader& reader) const { return m_read(reader, m_value); }
	};

	template<typename ... TField>
	inline HumanError struct_fields(Writer& writer, TField&& ... fields)
	{
		constexpr auto count = sizeof...(fields);
		if (count <= 15)
		{
			uint8_t prefix = 0x80 | (uint8_t)count;
			if (auto err = writer.write_uint8(prefix)) return err;
		}
		else if (count <= UINT16_MAX)
		{
			if (auto err = writer.write_uint8(0xde)) return err;
			if (auto err = writer.write_uint16((uint16_t)count)) return err;
		}
		else if (count <= UINT32_MAX)
		{
			if (auto err = writer.write_uint8(0xdf)) return err;
			if (auto err = writer.write_uint32((uint32_t)count)) return err;
		}
		else
		{
			coreUnreachable();
			return errf(writer.allocator(), "map is too long"_sv);
		}

		for (const auto& field: {fields...})
		{
			if (auto err = msgpack(writer, field.name())) return err;
			if (auto err = field.write(writer)) return err;
		}
		return {};
	}

	template<typename ... TField>
	inline HumanError struct_fields(Reader& reader, TField&& ... fields)
	{
		uint8_t prefix{};
		if (auto err = reader.read_uint8(prefix)) return err;

		size_t fields_count = 0;
		if (prefix >= 0x80 && prefix <= 0x8f)
		{
			fields_count = prefix & 0xf;
		}
		else if (prefix == 0xde)
		{
			uint16_t count{};
			if (auto err = reader.read_uint16(count)) return err;
			fields_count = count;
		}
		else if (prefix == 0xdf)
		{
			uint32_t count{};
			if (auto err = reader.read_uint32(count)) return err;
			fields_count = count;
		}
		else
		{
			return errf(reader.allocator(), "invalid map prefix"_sv);
		}

		String field_name{reader.allocator()};
		size_t required_fields = sizeof...(fields);
		for (size_t i = 0; i < fields_count; ++i)
		{
			if (auto err = msgpack(reader, field_name)) return err;

			bool consumed_value = false;
			for (const auto& f: {fields...})
			{
				if (f.name() == field_name)
				{
					if (auto err = f.read(reader)) return err;
					consumed_value = true;
					--required_fields;
					break;
				}
			}
			if (consumed_value == false)
				if (auto err = reader.skip()) return err;
		}

		if (required_fields != 0)
			return errf(reader.allocator(), "missing struct fields"_sv);

		return {};
	}

	class Value
	{
	public:
		enum KIND
		{
			KIND_NULL,
			KIND_BOOL,
			KIND_INT,
			KIND_UINT,
			KIND_FLOAT,
			KIND_DOUBLE,
			KIND_STRING,
			KIND_BYTES,
			KIND_ARRAY,
			KIND_MAP,
		};

		Value() = default;
		Value(nullptr_t) : m_kind{KIND_NULL} {}
		Value(bool value) : m_kind{KIND_BOOL}, m_bool{value} {}
		Value(int64_t value) : m_kind{KIND_INT}, m_int{value} {}
		Value(uint64_t value) : m_kind{KIND_UINT}, m_uint{value} {}
		Value(float value) : m_kind{KIND_FLOAT}, m_float{value} {}
		Value(double value) : m_kind{KIND_DOUBLE}, m_double{value} {}

		CORE_EXPORT Value(StringView value, Allocator* allocator);
		CORE_EXPORT Value(const std::byte* data, size_t size, Allocator* allocator);
		CORE_EXPORT Value(Array<Value> value);
		CORE_EXPORT Value(Map<String, Value> value);

		Value (const Value& other)
		{
			copyFrom(other);
		}

		Value (Value&& other)
		{
			moveFrom(std::move(other));
		}

		Value& operator=(const Value& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Value& operator=(Value&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		CORE_EXPORT Value& operator=(nullptr_t);
		CORE_EXPORT Value& operator=(bool value);
		CORE_EXPORT Value& operator=(int64_t value);
		CORE_EXPORT Value& operator=(uint64_t value);
		CORE_EXPORT Value& operator=(float value);
		CORE_EXPORT Value& operator=(double value);
		CORE_EXPORT Value& operator=(String value);
		CORE_EXPORT Value& operator=(Buffer value);
		CORE_EXPORT Value& operator=(Array<Value> value);
		CORE_EXPORT Value& operator=(Map<String, Value> value);

		Value& operator=(int8_t value) { return operator=((int64_t)value); }
		Value& operator=(int16_t value) { return operator=((int64_t)value); }
		Value& operator=(int32_t value) { return operator=((int64_t)value); }
		Value& operator=(uint8_t value) { return operator=((uint64_t)value); }
		Value& operator=(uint16_t value) { return operator=((uint64_t)value); }
		Value& operator=(uint32_t value) { return operator=((uint64_t)value); }

		~Value()
		{
			destroy();
		}

		KIND kind() const { return m_kind; }
		bool as_bool() const { coreAssert(m_kind == KIND_BOOL); return m_bool; }
		int64_t as_int() const { coreAssert(m_kind == KIND_INT); return m_int; }
		uint64_t as_uint() const { coreAssert(m_kind == KIND_UINT); return m_uint; }
		float as_float() const { coreAssert(m_kind == KIND_FLOAT); return m_float; }
		double as_double() const { coreAssert(m_kind == KIND_DOUBLE); return m_double; }

		String& as_string() { coreAssert(m_kind == KIND_STRING); return *m_string; }
		const String& as_string() const { coreAssert(m_kind == KIND_STRING); return *m_string; }

		Buffer& as_bytes() { coreAssert(m_kind == KIND_BYTES); return *m_bytes; }
		const Buffer& as_bytes() const { coreAssert(m_kind == KIND_BYTES); return *m_bytes; }

		Array<Value>& as_array() { coreAssert(m_kind == KIND_ARRAY); return *m_array; }
		const Array<Value>& as_array() const { coreAssert(m_kind == KIND_ARRAY); return *m_array; }

		Map<String, Value>& as_map() { coreAssert(m_kind == KIND_MAP); return *m_map; }
		const Map<String, Value>& as_map() const { coreAssert(m_kind == KIND_MAP); return *m_map; }

		CORE_EXPORT String release_string();
		CORE_EXPORT Buffer release_bytes();
		CORE_EXPORT Array<Value> release_array();
		CORE_EXPORT Map<String, Value> release_map();

	private:
		CORE_EXPORT void destroy();
		CORE_EXPORT void copyFrom(const Value& other);
		CORE_EXPORT void moveFrom(Value&& other);

		KIND m_kind = KIND_NULL;
		union
		{
			bool m_bool;
			int64_t m_int;
			uint64_t m_uint;
			float m_float;
			double m_double;
			String* m_string;
			Buffer* m_bytes;
			Array<Value>* m_array;
			Map<String, Value>* m_map;
		};
	};

	CORE_EXPORT HumanError msgpack(Writer& writer, const Value& value);
	CORE_EXPORT HumanError msgpack(Reader& reader, Value& value);

	template<typename T>
	inline static HumanError
	encode(Stream* stream, T& value, Allocator* allocator = nullptr)
	{
		Writer writer{stream, allocator};
		return msgpack(writer, value);
	}

	template<typename T>
	inline static HumanError
	decode(Stream* stream, T& value, Allocator* allocator = nullptr)
	{
		Reader reader{stream, allocator};
		return msgpack(reader, value);
	}
}