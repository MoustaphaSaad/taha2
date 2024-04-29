add_library(taha)

target_sources(taha PRIVATE
	include/taha/Engine.h
	src/taha/Engine.cpp
	include/taha/Frame.h
	src/taha/Frame.cpp
	include/taha/Renderer.h
	include/taha/NativeWindowDesc.h
	include/taha/Encoder.h
	src/taha/Encoder.cpp
	include/taha/Command.h
)

if (WIN32)
	target_sources(taha
		PRIVATE
			include/taha/dx11/DXPtr.h
			include/taha/dx11/DX11Renderer.h
			src/taha/dx11/DX11Renderer.cpp
			include/taha/dx11/DX11Frame.h
			src/taha/dx11/DX11Frame.cpp
	)
	target_compile_definitions(taha PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
	target_link_libraries(taha PRIVATE d3d11 dxgi d3dcompiler dxguid)
endif ()

target_link_libraries(taha PUBLIC core math)

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