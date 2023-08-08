#include "core/OSString.h"

namespace core
{
	OSString::OSString(StringView str, Allocator* allocator)
		: m_allocator(allocator)
	{
		// +1 for the null termination
		m_sizeInBytes = str.count() + 1;
		m_ptr = (char*)m_allocator->alloc(m_sizeInBytes, alignof(char));
		m_allocator->commit(m_ptr, m_sizeInBytes);
		memcpy(m_ptr, str.begin(), m_sizeInBytes);
		m_ptr[m_sizeInBytes - 1] = 0;
	}

	String OSString::toUtf8(Allocator* allocator) const
	{
		return String{StringView{m_ptr, m_sizeInBytes - 1}, allocator};
	}
}