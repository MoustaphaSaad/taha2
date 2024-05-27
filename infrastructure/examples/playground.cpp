#include <core/Stacktrace.h>
#include <core/File.h>

int main(int argc, char** argv)
{
	auto trace = core::Stacktrace::current(0, 10);
	trace.print(core::File::STDOUT, true);
	core::strf(core::File::STDOUT, "Hello, World!\n"_sv);
	return EXIT_SUCCESS;
}