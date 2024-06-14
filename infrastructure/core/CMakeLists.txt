add_library(core)

target_sources(core PRIVATE
  include/core/Allocator.h
  include/core/Mallocator.h
  include/core/VirtualMem.h
  include/core/FastLeak.h
  include/core/Unique.h
  include/core/Shared.h
  include/core/Array.h
  include/core/Hash.h
  include/core/String.h
  include/core/StringView.h
  include/core/Rune.h
  include/core/Stream.h
  include/core/File.h
  include/core/OSString.h
  include/core/Result.h
  include/core/Buffer.h
  include/core/MemoryStream.h
  include/core/Intrinsics.h
  include/core/Msgpack.h
  include/core/UUID.h
  include/core/Func.h
  include/core/Thread.h
  include/core/ConditionVariable.h
  include/core/Queue.h
  include/core/NotificationQueue.h
  include/core/ThreadPool.h
  include/core/WaitGroup.h
  include/core/Log.h
  include/core/Url.h
  include/core/Socket.h
  include/core/Span.h
  include/core/Base64.h
  include/core/SHA1.h
  include/core/Rand.h
  include/core/ExecutionQueue.h
  include/core/Stacktrace.h
  include/core/Assert.h
  include/core/Chan.h
  include/core/OS.h
  include/core/Lock.h
  include/core/Path.h
  include/core/CUtils.h
  include/core/BufferedReader.h
  include/core/Mimallocator.h
  include/core/ws/Message.h
  include/core/ws/Client.h
  include/core/ws/Handshake.h
  include/core/ws/Server.h
  src/core/StringView.cpp
  src/core/Mallocator.cpp
  src/core/FastLeak.cpp
  src/core/String.cpp
  src/core/Rune.cpp
  src/core/File.cpp
  src/core/Buffer.cpp
  src/core/MemoryStream.cpp
  src/core/Msgpack.cpp
  src/core/UUID.cpp
  src/core/ThreadPool.cpp
  src/core/Url.cpp
  src/core/Base64.cpp
  src/core/SHA1.cpp
  src/core/Assert.cpp
  src/core/Mimallocator.cpp
  src/core/ws/Message.cpp
  src/core/ws/Client.cpp
  src/core/ws/Handshake.cpp
  src/core/ws/Server.cpp
)

if (WIN32)
  target_sources(core
    PRIVATE
    src/core/winos/VirtualMem.cpp
    src/core/winos/File.cpp
    src/core/winos/OSString.cpp
    src/core/winos/Intrinsics.cpp
    src/core/winos/Mutex.cpp
    src/core/winos/Thread.cpp
    src/core/winos/ConditionVariable.cpp
    src/core/winos/IMutex.h
    src/core/winos/Socket.cpp
    src/core/winos/Rand.cpp
    src/core/winos/OS.cpp
    src/core/winos/IPCMutex.cpp
    src/core/winos/Path.cpp
  )
  target_compile_definitions(core PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
elseif (APPLE)
  target_sources(core
    PRIVATE
    src/core/macos/VirtualMem.cpp
    src/core/macos/File.cpp
    src/core/macos/OSString.cpp
    src/core/macos/Intrinsics.cpp
    src/core/macos/Mutex.cpp
    src/core/macos/Thread.cpp
    src/core/macos/ConditionVariable.cpp
    src/core/macos/IMutex.h
    src/core/macos/Socket.cpp
    src/core/macos/Rand.cpp
    src/core/macos/OS.cpp
    src/core/macos/IPCMutex.cpp
    src/core/macos/Path.cpp
    src/core/macos/Path.mm
  )
  target_compile_definitions(core PRIVATE _LARGEFILE64_SOURCE)
  find_library(FOUNDATION_LIBRARY Foundation)
  target_link_libraries(core PRIVATE ${FOUNDATION_LIBRARY})
elseif (UNIX AND NOT APPLE)
  target_sources(core
    PRIVATE
    src/core/linux/VirtualMem.cpp
    src/core/linux/File.cpp
    src/core/linux/OSString.cpp
    src/core/linux/Intrinsics.cpp
    src/core/linux/Mutex.cpp
    src/core/linux/Thread.cpp
    src/core/linux/ConditionVariable.cpp
    src/core/linux/IMutex.h
    src/core/linux/Socket.cpp
    src/core/linux/Rand.cpp
    src/core/linux/OS.cpp
    src/core/linux/IPCMutex.cpp
    src/core/linux/Path.cpp
  )
  target_compile_definitions(core PRIVATE _LARGEFILE64_SOURCE)
endif ()

CPMGetPackage(utf8proc)
CPMGetPackage(fmt)
CPMGetPackage(spdlog)
CPMGetPackage(libressl)
CPMGetPackage(tracy)
CPMGetPackage(mimalloc)
if (UNIX)
#  compression is disabled in libdwarf at the moment
#  CPMGetPackage(zlib)
#  CPMGetPackage(zstd)
  CPMGetPackage(libdwarf)
#  target_include_directories(dwarf PRIVATE ${zstd_SOURCE_DIR}/lib)
#  target_include_directories(dwarf PRIVATE ${zlib_SOURCE_DIR})
#  add_dependencies(dwarf libzstd_static libzstd_shared)
#  add_dependencies(dwarf zlib zlibstatic)
endif ()
CPMGetPackage(cpptrace-lib)
target_link_libraries(core PUBLIC utf8proc fmt spdlog ssl crypto tls TracyClient cpptrace-lib mimalloc)
target_link_libraries(core
  PRIVATE
  "$<$<PLATFORM_ID:Windows>:bcrypt;ws2_32>"
  "$<$<PLATFORM_ID:Linux>:pthread>"
  "$<$<PLATFORM_ID:Darwin>:pthread>"
)
target_compile_definitions(core
  PUBLIC
  "$<$<CONFIG:DEBUG>:DEBUG>"
  -DTAHA_ENABLE_ASSERTS=$<BOOL:${ENABLE_ASSERTS}>
  $<$<CXX_COMPILER_ID:Clang>:TAHA_COMPILER_CLANG=1>
  $<$<CXX_COMPILER_ID:AppleClang>:TAHA_COMPILER_CLANG=1>
  $<$<CXX_COMPILER_ID:GNU>:TAHA_COMPILER_GNU=1>
  $<$<CXX_COMPILER_ID:MSVC>:TAHA_COMPILER_MSVC=1>
  $<$<PLATFORM_ID:Windows>:TAHA_OS_WINDOWS=1>
  $<$<PLATFORM_ID:Linux>:TAHA_OS_LINUX=1>
  $<$<PLATFORM_ID:Darwin>:TAHA_OS_DARWIN=1>
)

target_compile_features(core PUBLIC cxx_std_20)
set_target_properties(core PROPERTIES
  CXX_EXTENSIONS OFF
  DEBUG_POSTFIX d
)

# generate exports header file
include(GenerateExportHeader)
generate_export_header(core
  EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/core/Exports.h
)

target_include_directories(core
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)