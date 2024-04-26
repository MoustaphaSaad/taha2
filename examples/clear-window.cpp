#include <core/FastLeak.h>
#include <core/Log.h>

#include <GLFW/glfw3.h>
#if TAHA_OS_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32 1
#endif
#include <GLFW/glfw3native.h>

#include <taha/Engine.h>

taha::NativeWindowDesc getNativeWindowDesc(GLFWwindow* window)
{
	int windowWidth = 0;
	int windowHeight = 0;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	taha::NativeWindowDesc nativeWindowDesc{};
	#if TAHA_OS_WINDOWS
	nativeWindowDesc.windowHandle = glfwGetWin32Window(window);
	#endif
	nativeWindowDesc.width = windowWidth;
	nativeWindowDesc.height = windowHeight;

	return nativeWindowDesc;
}

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

	auto nativeWindowDesc = getNativeWindowDesc(window);
	auto frame = engine.createFrameForWindow(nativeWindowDesc);

	glfwMakeContextCurrent(window);
	while (glfwWindowShouldClose(window) == false)
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return EXIT_SUCCESS;
}