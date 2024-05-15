#pragma once

#include "core/Mutex.h"

#include <Windows.h>

namespace core
{
	struct Mutex::IMutex
	{
		CRITICAL_SECTION cs;
	};
}