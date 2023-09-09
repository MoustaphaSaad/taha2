add_library(sabre)

target_sources(sabre
	PRIVATE
		include/sabre/Dummy.h
		include/sabre/Token.h
		src/sabre/Dummy.cpp
)

target_link_libraries(sabre PUBLIC core)

target_compile_features(sabre PUBLIC cxx_std_17)
set_target_properties(sabre PROPERTIES
	CXX_EXTENSIONS OFF
	DEBUG_POSTFIX d
)

# generate exports header file
include(GenerateExportHeader)
generate_export_header(sabre
	EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/sabre/Exports.h
)

target_include_directories(sabre
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)