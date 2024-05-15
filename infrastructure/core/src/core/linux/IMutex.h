#pragma once

#include "core/Mutex.h"

#include <pthread.h>

namespace core
{
	struct Mutex::IMutex
	{
		pthread_mutex_t handle;
	};
}