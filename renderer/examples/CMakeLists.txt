CPMGetPackage(glfw)
add_executable(window window.cpp)
target_link_libraries(window core glfw)

CPMGetPackage(glfw)
add_executable(clear-window clear-window.cpp)
target_link_libraries(clear-window taha glfw)

CPMGetPackage(nanobench)
add_executable(bench-model-transform bench-model-transform.cpp)
target_link_libraries(bench-model-transform core math nanobench)

if (WIN32)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif (UNIX AND NOT APPLE)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR;VK_USE_PLATFORM_XLIB_KHR)
endif ()
CPMGetPackage(glfw)
CPMGetPackage(volk)
CPMGetPackage(glm)
add_executable(vulkan-tutorial vulkan-tutorial.cpp)
target_link_libraries(vulkan-tutorial core math glfw volk glm)
target_compile_definitions(vulkan-tutorial PUBLIC
  -DSHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")