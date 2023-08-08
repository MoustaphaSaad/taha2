#include "core/OSString.h"

#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace core
{
	void OSString::destroy()
	{
		if (m_ptr)
		{
			m_allocator->release(m_ptr, m_sizeInBytes);
			m_allocator->free(m_ptr, m_sizeInBytes);
			m_ptr = nullptr;
			m_sizeInBytes = 0;
		}
	}

	void OSString::copyFrom(const OSString& other)
	{
		m_allocator = other.m_allocator;
		m_sizeInBytes = other.m_sizeInBytes;
		m_ptr = (char*)m_allocator->alloc(m_sizeInBytes, alignof(TCHAR));
		m_allocator->commit(m_ptr, m_sizeInBytes);
		memcpy(m_ptr, other.m_ptr, m_sizeInBytes);
	}

	void OSString::moveFrom(OSString&& other)
	{
		m_allocator = other.m_allocator;
		m_sizeInBytes = other.m_sizeInBytes;
		m_ptr = other.m_ptr;

		other.m_allocator = nullptr;
		other.m_sizeInBytes = 0;
		other.m_ptr = nullptr;
	}

	OSString::OSString(StringView str, Allocator* allocator)
		: m_allocator(allocator)
	{
		auto countNeeded = MultiByteToWideChar(CP_UTF8, 0, str.begin(), str.count(), nullptr, 0);

		// +1 for the null termination
		m_sizeInBytes = (countNeeded + 1) * sizeof(TCHAR);
		m_ptr = (char*)m_allocator->alloc(m_sizeInBytes, alignof(TCHAR));
		m_allocator->commit(m_ptr, m_sizeInBytes);
		MultiByteToWideChar(CP_UTF8, 0, str.begin(), str.count(), (TCHAR*)m_ptr, countNeeded);

		auto ptr = (TCHAR*)m_ptr;
		ptr[countNeeded] = TCHAR{0};
	}

	String OSString::toUtf8(Allocator* allocator) const
	{
		auto countNeeded = WideCharToMultiByte(CP_UTF8, NULL, (LPTSTR)m_ptr, int(m_sizeInBytes / sizeof(TCHAR)), NULL, 0, NULL, NULL);
		if (countNeeded == 0)
			return String{allocator};

		String str{allocator};
		// countNeeded includes null byte
		str.resize(countNeeded - 1);

		countNeeded = WideCharToMultiByte(CP_UTF8, NULL, (LPTSTR)m_ptr, int(m_sizeInBytes / sizeof(TCHAR)), str.m_ptr, int(str.m_count), NULL, NULL);
		return str;
	}
}