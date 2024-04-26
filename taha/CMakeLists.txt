add_library(taha)

target_sources(taha PRIVATE
	include/taha/Engine.h
	src/taha/Engine.cpp
	include/taha/Frame.h
	src/taha/Frame.cpp
)

target_link_libraries(taha PUBLIC core)

# generate exports header file
include(GenerateExportHeader)
generate_export_header(taha
	EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/taha/Exports.h
)

target_include_directories(taha
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
)