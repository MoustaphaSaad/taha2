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
	include/core/websocket/Handshake.h
	include/core/websocket/Websocket.h
	include/core/websocket/FrameParser.h
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
	src/core/websocket/Handshake.cpp
	src/core/websocket/FrameParser.cpp
)

if (WIN32)
	target_sources(core
		PRIVATE
			src/core/winos/VirtualMem.cpp
			src/core/winos/File.cpp
			src/core/winos/OSString.cpp
			src/core/winos/Intrinsics.cpp
			src/core/winos/UUID.cpp
			src/core/winos/Mutex.cpp
			src/core/winos/Thread.cpp
			src/core/winos/ConditionVariable.cpp
			src/core/winos/IMutex.h
			src/core/winos/Socket.cpp
			src/core/winos/websocket/Websocket.cpp
	)
	target_compile_definitions(core PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
elseif (APPLE)
	target_sources(core
		PRIVATE
			src/core/macos/VirtualMem.cpp
			src/core/macos/File.cpp
			src/core/macos/OSString.cpp
			src/core/macos/Intrinsics.cpp
			src/core/macos/UUID.cpp
			src/core/macos/Mutex.cpp
			src/core/macos/Thread.cpp
			src/core/macos/ConditionVariable.cpp
			src/core/macos/IMutex.h
			src/core/macos/Socket.cpp
	)
	target_compile_definitions(core PRIVATE _LARGEFILE64_SOURCE)
elseif (UNIX AND NOT APPLE)
	target_sources(core
		PRIVATE
			src/core/linux/VirtualMem.cpp
			src/core/linux/File.cpp
			src/core/linux/OSString.cpp
			src/core/linux/Intrinsics.cpp
			src/core/linux/UUID.cpp
			src/core/linux/Mutex.cpp
			src/core/linux/Thread.cpp
			src/core/linux/ConditionVariable.cpp
			src/core/linux/IMutex.h
			src/core/linux/Socket.cpp
	)
	target_compile_definitions(core PRIVATE _LARGEFILE64_SOURCE)
endif ()

CPMGetPackage(utf8proc)
CPMGetPackage(fmt)
CPMGetPackage(spdlog)
CPMGetPackage(libressl)
CPMGetPackage(tracy)
target_link_libraries(core PUBLIC utf8proc fmt spdlog ssl crypto tls TracyClient)
target_link_libraries(core
	PRIVATE
		"$<$<PLATFORM_ID:Windows>:bcrypt;ws2_32>"
		"$<$<PLATFORM_ID:Linux>:pthread>"
		"$<$<PLATFORM_ID:Darwin>:pthread>"
)
target_compile_definitions(core
	PUBLIC
		"$<$<CONFIG:DEBUG>:DEBUG>"
)

target_compile_features(core PUBLIC cxx_std_17)
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