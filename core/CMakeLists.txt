add_library(core)

target_sources(core
	PRIVATE
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
		src/core/StringView.cpp
		src/core/Mallocator.cpp
		src/core/FastLeak.cpp
		src/core/String.cpp
		src/core/Rune.cpp
)

if (WIN32)
	target_sources(core
		PRIVATE
			src/core/winos/VirtualMem.cpp
			src/core/winos/File.cpp
			src/core/winos/OSString.cpp
	)
	target_compile_definitions(core PRIVATE UNICODE _UNICODE)
elseif (APPLE)
	target_sources(core
		PRIVATE
			src/core/macos/VirtualMem.cpp
	)
elseif (UNIX AND NOT APPLE)
	target_sources(core
		PRIVATE
			src/core/linux/VirtualMem.cpp
	)
endif ()

CPMGetPackage(utf8proc)
CPMGetPackage(fmt)
target_link_libraries(core PUBLIC utf8proc fmt)

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
)