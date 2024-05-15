add_library(fin)

target_sources(fin PRIVATE
	include/fin/Server.h
	src/fin/Server.cpp
	include/fin/Ledger.h
	src/fin/Ledger.cpp
)

target_link_libraries(fin PUBLIC core sqlite3)

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