# ================================================================================
# 基础配置区
# ================================================================================
cmake_minimum_required(VERSION 3.16)
project(QtForge VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================================================================================
# 通用配置
# ================================================================================
set(CMAKE_WARN_UNUSED_CLI YES)

if(NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/utf-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/bigobj>")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/bigobj>")
endif()

# ================================================================================
# 查找依赖
# ================================================================================
find_package(spdlog CONFIG REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)

# ================================================================================
# 内置库
# ================================================================================
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/XlcLogger")
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/XlcLogWidget")

# ================================================================================
# 示例程序
# ================================================================================
option(QTFORGE_BUILD_EXAMPLES "Build QtForge example applications" ON)

if(QTFORGE_BUILD_EXAMPLES)
    add_subdirectory("${CMAKE_SOURCE_DIR}/examples/log_widget_demo")
endif()
