# =============================================================================
# XlcLogWidget — QListView 展示 spdlog 日志（静态库）
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(XlcLogWidget LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog CONFIG REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# 含 Q_OBJECT 的头文件须列入目标源文件，AUTOMOC 才会生成并链入 moc_*.cpp（静态库尤其需要）
set(CMAKE_AUTOMOC ON)

add_library(XlcLogWidget STATIC
    src/XlcLogWidget.cpp
    include/XlcLogWidget.h
)

target_include_directories(XlcLogWidget
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(XlcLogWidget
    PUBLIC
        Qt5::Core
        Qt5::Widgets
        spdlog::spdlog
)

set_target_properties(XlcLogWidget PROPERTIES
    AUTOMOC ON
)

if(WIN32)
    target_compile_definitions(XlcLogWidget PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    target_compile_options(XlcLogWidget PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>"
    )
endif()
