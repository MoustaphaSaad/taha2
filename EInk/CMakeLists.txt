cmake_minimum_required(VERSION 3.16)

project(EInk VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.7 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.7)

qt_add_library(EInk STATIC)
set_source_files_properties(Style.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(EInk
	URI EInk
	VERSION "${PROJECT_VERSION}"
	QML_FILES
		ApplicationWindow.qml
		Style.qml
		EInk.qml
		Icon.qml
		Button.qml
		ButtonSizeSpec.qml
		ButtonSpec.qml
		ButtonStyleSpec.qml
		DefaultButton.qml
		Ripple.qml
)

target_link_libraries(EInk PRIVATE
	Qt6::Quick)
