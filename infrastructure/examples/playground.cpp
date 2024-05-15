#include <core/File.h>

int main(int argc, char** argv)
{
	core::strf(core::File::STDOUT, "Hello, World!\n"_sv);
	return EXIT_SUCCESS;
}