#pragma once

namespace core
{
	constexpr size_t strlen(const char* start)
	{
		auto end = start;
		while (*end != '\0')
		{
			++end;
		}
		return end - start;
	}

	template <typename F>
	struct DeferClosure
	{
		F f;
		DeferClosure(F f)
			: f(f)
		{}
		~DeferClosure()
		{
			f();
		}
	};

#define coreDefer1(x, y) x##y
#define coreDefer2(x, y) coreDefer1(x, y)
#define coreDefer3(x) coreDefer2(x, __COUNTER__)

#define coreDefer core::DeferClosure coreDefer3(_defer_) = [&]()
}