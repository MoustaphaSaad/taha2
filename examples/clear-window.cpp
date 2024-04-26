#include <core/FastLeak.h>
#include <core/Log.h>

#include <GLFW/glfw3.h>

#include <taha/Engine.h>

int main()
{
	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto engineResult = taha::Engine::create(&allocator);
	if (engineResult.isError())
	{
		log.critical("taha::Engine::create failed, {}"_sv, engineResult.error());
		return EXIT_FAILURE;
	}
	auto engine = engineResult.releaseValue();

	if (glfwInit() == false)
	{
		log.critical("glfwInit failed"_sv);
		return EXIT_FAILURE;
	}

	auto window = glfwCreateWindow(640, 480, "Clear Window", nullptr, nullptr);
	if (window == nullptr)
	{
		log.critical("glfwCreateWindow failed"_sv);
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