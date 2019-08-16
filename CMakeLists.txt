cmake_minimum_required(VERSION 3.15.0)

project(mdoctor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

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
target_link_libraries(mdoctor Qt5::Widgets Qt5::Concurrent)
