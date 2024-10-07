#pragma once

#if TAHA_OS_WINDOWS
	#include <Windows.h>
#elif TAHA_OS_LINUX
	#include <wayland-client.h>
#endif

namespace taha
{
	struct NativeWindowDesc
	{
#if TAHA_OS_WINDOWS
		HWND windowHandle = nullptr;
#elif TAHA_OS_LINUX
		wl_display* display;
		wl_surface* surface;
#endif
		int width = 0;
		int height = 0;
		bool enableVSync = true;
	};
}