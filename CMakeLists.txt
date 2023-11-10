cmake_minimum_required(VERSION 3.9)

project(taha2 VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SHARED_LIBS "Build all libraries as shared" ON)

include(CPM)
CPMUsePackageLock(package-lock.cmake)

add_subdirectory(external)
add_subdirectory(core)
add_subdirectory(math)
add_subdirectory(sabre)
add_subdirectory(test)
add_subdirectory(examples)