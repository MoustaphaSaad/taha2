#include <core/FastLeak.h>
#include <core/Log.h>

#include <taha/Engine.h>

#include <GLFW/glfw3.h>
#if TAHA_OS_WINDOWS
	#define GLFW_EXPOSE_NATIVE_WIN32 1
	#define NOMINMAX 1
	#define WIN32_LEAN_AND_MEAN 1
	#define _CRT_SECURE_NO_WARNINGS 1
#elif TAHA_OS_LINUX
	#define GLFW_EXPOSE_NATIVE_WAYLAND 1
#endif
#include <GLFW/glfw3native.h>

taha::NativeWindowDesc getNativeWindowDesc(GLFWwindow* window)
{
	int windowWidth = 0;
	int windowHeight = 0;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	taha::NativeWindowDesc nativeWindowDesc{};
#if TAHA_OS_WINDOWS
	nativeWindowDesc.windowHandle = glfwGetWin32Window(window);
#elif TAHA_OS_LINUX
	nativeWindowDesc.display = glfwGetWaylandDisplay();
	nativeWindowDesc.surface = glfwGetWaylandWindow(window);
#endif
	nativeWindowDesc.width = windowWidth;
	nativeWindowDesc.height = windowHeight;

	return nativeWindowDesc;
}

int main()
{
	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto engineResult = taha::Engine::create(taha::Engine::API_VULKAN, &log, &allocator);
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

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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
		auto encoder = frame->createEncoderAndRecord(
			{
				.color = {taha::FrameColorAction{.value = {1, 1, 0, 1}}},
			},
			&allocator);
		encoder.endEncodingAndSubmit();

		glfwPollEvents();
	}

	glfwTerminate();
	return EXIT_SUCCESS;
}