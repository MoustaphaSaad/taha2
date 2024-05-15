#include <core/FastLeak.h>
#include <core/Log.h>
#include <GLFW/glfw3.h>

int main()
{
	core::FastLeak allocator{};
	core::Log log{&allocator};

	if (!glfwInit())
	{
		log.critical("failed to init glfw"_sv);
		return EXIT_FAILURE;
	}

	auto window = glfwCreateWindow(640, 480, "Mostafa", nullptr, nullptr);
	if (!window)
	{
		log.critical("failed to create glfw window"_sv);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);

	while (glfwWindowShouldClose(window) == false)
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return EXIT_SUCCESS;
}