cmake_minimum_required(VERSION 3.12.0)

project(mdoctor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_MACOSX_BUNDLE ON)

find_package(Qt5 COMPONENTS Widgets Concurrent REQUIRED)

set(MDOCTOR_SOURCES
    MainWindow.ui
    MainWindow.cpp
    MainWindow.h
    Main.cpp
    Lexer.cpp
    Lexer.h
    Parser.cpp
    Parser.h
    magic_enum.hpp
    ElementUI.cpp
    ElementUI.h
    GraphicsView.cpp
    GraphicsView.h
    ConnectionUI.cpp
    ConnectionUI.h
    ClickGraphScene.cpp
    ClickGraphScene.h
    FlatConfigToScene.cpp
    FlatConfigToScene.h
    GraphLayoutHelper.cpp
    GraphLayoutHelper.h
    NavigationPane.cpp
    NavigationPane.h
    TargetDevice.cpp
    TargetDevice.h
    TraceDat.cpp
    TraceDat.h
    TraceEventClick.cpp
    TraceEventClick.h
)
add_executable(mdoctor WIN32 ${MDOCTOR_SOURCES})
set_target_properties(mdoctor PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME mdoctor
    MACOSX_BUNDLE_BUNDLE_VERSION 1.0.2
    MACOSX_BUNDLE_COPYRIGHT "All rights reserved"
    MACOSX_BUNDLE_GUI_IDENTIFIER com.meraki.tchen.mdoctor
    #MACOSX_BUNDLE_ICON_FILE
    #MACOSX_BUNDLE_INFO_STRING
    #MACOSX_BUNDLE_LONG_VERSION_STRING
    #MACOSX_BUNDLE_SHORT_VERSION_STRING
)
target_link_libraries(mdoctor Qt5::Widgets Qt5::Concurrent)

get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
if(WINDEPLOYQT_EXECUTABLE)
add_custom_command(TARGET mdoctor POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E env PATH="${_qt_bin_dir}"
        "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:mdoctor>"
    COMMENT "Running windeployqt"
)
endif()
if(MACDEPLOYQT_EXECUTABLE)
add_custom_command(TARGET mdoctor POST_BUILD
    COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_BUNDLE_DIR:mdoctor>" -always-overwrite
    COMMENT "Running macdeployqt"
)
endif()

# Installation
install(TARGETS mdoctor
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
include(InstallRequiredSystemLibraries)

# Packaging
set(CPACK_PACKAGE_NAME "mdoctor")
set(CPACK_PACKAGE_VENDOR "Meraki")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 2)
set(CPACK_GENERATOR DragNDrop 7Z)
include(CPack)
