add_library(ledger)

target_sources(ledger PRIVATE
	include/ledger/Ledger.h
	src/ledger/Ledger.cpp
)

target_link_libraries(ledger PUBLIC core sqlite3)
set_target_properties(ledger PROPERTIES
	CXX_EXTENSIONS OFF
	DEBUG_POSTFIX d
)

include(GenerateExportHeader)
generate_export_header(ledger
	EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/ledger/Exports.h
)

target_include_directories(ledger
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)