#include <core/File.h>

int main()
{
	core::strf(core::File::STDOUT, "Hello, World!\n"_sv);
	return 0;
}