add_library(core)

target_sources(core
	PRIVATE
		include/core/Allocator.h
		include/core/Mallocator.h
		include/core/VirtualMem.h
		include/core/FastLeak.h
		include/core/Unique.h
		include/core/Shared.h
		src/core/Mallocator.cpp
		src/core/FastLeak.cpp
)

if (WIN32)
	target_sources(core
		PRIVATE
			src/core/winos/VirtualMem.cpp
	)
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