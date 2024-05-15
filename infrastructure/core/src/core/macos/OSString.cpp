#include "core/OSString.h"

namespace core
{
	OSString::OSString(StringView str, Allocator* allocator)
		: m_buffer(allocator)
	{
		m_buffer.resize(str.count() + 1);
		::memcpy(m_buffer.data(), str.data(), str.count());
		m_buffer[str.count()] = std::byte{0};
	}

	String OSString::toUtf8(Allocator* allocator) const
	{
		return String{StringView{(const char*)m_buffer.data(), m_buffer.count() - 1}, allocator};
	}
}