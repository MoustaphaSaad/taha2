#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/Stacktrace.h>
#include <core/File.h>

int main(int argc, char** argv)
{
	core::Mallocator allocator{};
	core::Array<core::Rawtrace> traces{&allocator};
	for (size_t i = 0; i < 10000; ++i)
	{
		traces.push(core::Rawtrace::current(0, 20));
	}
	auto& trace = traces[rand()%traces.count()];
	trace.resolve().print(core::File::STDOUT, true);
	return EXIT_SUCCESS;
}