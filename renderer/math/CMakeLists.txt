add_library(math INTERFACE)

target_compile_features(math INTERFACE cxx_std_20)
target_link_libraries(math INTERFACE core)

target_include_directories(math INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)