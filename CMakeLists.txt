cmake_minimum_required(VERSION 3.9)

project(taha2 VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SHARED_LIBS "Build all libraries as shared" ON)
option(TRACY_ENABLE "Build all tests" OFF)
option(ADDRESS_SANITIZER "Enables address sanitizer" OFF)
option(THREAD_SANITIZER "Enables thread sanitizer" OFF)
option(UNDEFINED_SANITIZER "Enables undefined behavior sanitizer" OFF)
option(ENABLE_ASM "Enables inline assembly code embedded in libressl" ON)
option(ENABLE_ASSERTS "Enables assert checks" ON)

if (ADDRESS_SANITIZER AND THREAD_SANITIZER)
	message(FATAL_ERROR "address sanitizer and thread sanitizer cannot run at the same time")
endif ()

if (ADDRESS_SANITIZER)
	add_compile_options(-fsanitize=address)
	add_link_options(-fsanitize=address)
endif()

if (UNDEFINED_SANITIZER)
	add_compile_options(-fsanitize=undefined)
	add_link_options(-fsanitize=undefined)
endif()

if (THREAD_SANITIZER)
	add_compile_options(-fsanitize=thread)
	add_link_options(-fsanitize=thread)
endif()

include(CPM)
CPMUsePackageLock(package-lock.cmake)

add_subdirectory(external)
add_subdirectory(core)
add_subdirectory(math)
add_subdirectory(sabre)
add_subdirectory(test)
add_subdirectory(examples)
add_subdirectory(budget-byte)
add_subdirectory(budget-byte-cli)
add_subdirectory(fin)
add_subdirectory(fin-server)
add_subdirectory(EInk)
add_subdirectory(fin-gui)
add_subdirectory(taha)