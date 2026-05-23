# ================================================================================
# 导入cmake模块
# ================================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
# ==================== Qt查找函数 ====================
include(FindMyQt)


# ================================================================================
# 基础配置区
# ================================================================================
cmake_minimum_required(VERSION 3.16)
project(CMake_Qt_Project_Example VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================================================================================
# 查找 & 链接库
# ================================================================================
# ==================== 找包 ====================
find_package(spdlog CONFIG REQUIRED)
find_my_qt(5)
find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)

add_subdirectory("${CMAKE_SOURCE_DIR}/libs/XlcLogger")
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/XlcLogWidget")

# ==================== 整合资源 ====================
file(GLOB_RECURSE HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src" SOURCES)
set(RESOURCES
)

# ==================== 配置可执行文件 ====================
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Qt 自动代码生成
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# ==================== 链接库 ====================
target_link_libraries(${PROJECT_NAME} PRIVATE
    XlcLogger
    XlcLogWidget
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:XlcLogger>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying XlcLogger runtime library to executable directory"
    )
endif()

# ==================== 指定头文件路径 ====================
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# SPDLOG_ACTIVE_LEVEL 由 XlcLogger 以 PUBLIC 方式传递；若需覆盖可在本目标上再写 target_compile_definitions

# ================================================================================
# 配置区
# ================================================================================
# ==================== 通用配置 ====================
set(CMAKE_WARN_UNUSED_CLI YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# release版本自动隐藏控制台
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE $<$<CONFIG:Release>:TRUE>
)

# ==================== 自定义配置 ====================

# ================================================================================
# 自定义功能开关区域
# ================================================================================
