#pragma once

#if TAHA_OS_WINDOWS
#include <Windows.h>
#endif

namespace taha
{
	struct NativeWindowDesc
	{
		#if TAHA_OS_WINDOWS
		HWND windowHandle = nullptr;
		#endif
		int width = 0;
		int height = 0;
		bool enableVSync = true;
	};
}