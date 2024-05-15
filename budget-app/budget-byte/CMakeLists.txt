add_library(budget-byte)

target_sources(budget-byte PRIVATE
	include/budget-byte/Ledger.h
	src/budget-byte/Ledger.cpp
)

target_link_libraries(budget-byte PUBLIC core sqlite3)
set_target_properties(budget-byte PROPERTIES
	CXX_EXTENSIONS OFF
	DEBUG_POSTFIX d
)

include(GenerateExportHeader)
generate_export_header(budget-byte
	EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/budget-byte/Exports.h
)

target_include_directories(budget-byte
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)