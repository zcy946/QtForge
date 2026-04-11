# =============================================================================
# XlcLogger — 基于 spdlog 的日志库
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(XlcLogger LANGUAGES CXX VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog CONFIG REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)

add_library(XlcLogger SHARED
    src/XlcLogger.cpp
)

target_include_directories(XlcLogger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(XlcLogger
    PUBLIC
        Qt5::Core
        spdlog::spdlog
)

# MSVC / MinGW：从 DLL 导出符号（实现主要在 .cpp，模板在头文件中内联）
set_target_properties(XlcLogger PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# 与主工程一致：编译期日志级别（影响 LOG_* 宏是否裁掉低级别）
target_compile_definitions(XlcLogger PUBLIC
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=0>
    $<$<CONFIG:RelWithDebInfo>:SPDLOG_ACTIVE_LEVEL=0>
    $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=2>
    $<$<CONFIG:MinSizeRel>:SPDLOG_ACTIVE_LEVEL=2>
)

if(WIN32)
    target_compile_definitions(XlcLogger PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    target_compile_options(XlcLogger PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>"
    )
endif()
