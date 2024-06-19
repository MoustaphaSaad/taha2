CPMGetPackage(glfw)
add_executable(window window.cpp)
target_link_libraries(window core glfw)

CPMGetPackage(glfw)
add_executable(clear-window clear-window.cpp)
target_link_libraries(clear-window taha glfw)

CPMGetPackage(nanobench)
add_executable(bench-model-transform bench-model-transform.cpp)
target_link_libraries(bench-model-transform core math nanobench)

CPMGetPackage(glfw)
CPMGetPackage(volk)
add_executable(vulkan-tutorial vulkan-tutorial.cpp)
target_link_libraries(vulkan-tutorial core math glfw volk)