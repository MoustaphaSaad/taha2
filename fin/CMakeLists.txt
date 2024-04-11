add_library(fin)

target_sources(fin PRIVATE
	include/fin/Server.h
	src/fin/Server.cpp
)

target_link_libraries(fin PUBLIC core)

# generate exports header file
include(GenerateExportHeader)
generate_export_header(fin
	EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/fin/Exports.h
)

target_include_directories(fin
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
)