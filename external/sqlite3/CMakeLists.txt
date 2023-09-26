add_library(sqlite3)

target_sources(sqlite3
	PRIVATE
		sqlite3.h
		sqlite3.c
		sqlite3ext.h
		shell.c
)

set_target_properties(sqlite3 PROPERTIES
	CXX_EXTENSIONS OFF
	DEBUG_POSTFIX d
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(sqlite3
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)