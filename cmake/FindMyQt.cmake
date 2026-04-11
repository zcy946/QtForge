# cmake/FindMyQt.cmake

function(find_my_qt TARGET_QT_VERSION)
    # 确定系统位数后缀
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH "X64")
    else()
        set(ARCH "X32")
    endif()

    # 确定编译器类型
    if(MSVC)
        set(COMPILER "MSVC")
    elseif(MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        set(COMPILER "MINGW")
    else()
        set(COMPILER "UNKNOWN")
    endif()

    # 电脑上的Qt路径映射(vscode中对应的profile内已定义这些信息)
    if(NOT ROOT_QT_5_MSVC_X64)
        set(ROOT_QT_5_MSVC_X64 "D:/Software/Development/Qt/5.15.2/msvc2019_64")
    endif()
    if(NOT ROOT_QT_5_MSVC_X32)
        set(ROOT_QT_5_MSVC_X32 "D:/Software/Development/Qt/5.15.2/msvc2019")
    endif()
    if(NOT ROOT_QT_6_MSVC_X64)
        set(ROOT_QT_6_MSVC_X64 "D:/Software/Development/Qt/6.8.0/msvc2019_64")
    endif()
    if(NOT ROOT_QT_5_MINGW_X64)
        set(ROOT_QT_5_MINGW_X64 "你的Qt安装路径")
    endif()
    if(NOT ROOT_QT_5_MINGW_X32)
        set(ROOT_QT_5_MINGW_X32 "你的Qt安装路径")
    endif()
    if(NOT ROOT_QT_6_MINGW_X64)
        set(ROOT_QT_6_MINGW_X64 "D:/Software/Development/Qt/6.8.0/mingw_64")
    endif()

    set(TARGET_VAR_NAME "ROOT_QT_${TARGET_QT_VERSION}_${COMPILER}_${ARCH}")

    # 设置 CMAKE_PREFIX_PATH
    if(DEFINED ${TARGET_VAR_NAME})
        list(APPEND CMAKE_PREFIX_PATH ${${TARGET_VAR_NAME}})
        message(STATUS "Auto-Config: Using Qt ${TARGET_QT_VERSION} (${COMPILER}/${ARCH}) at ${${TARGET_VAR_NAME}}")
        # 将结果返回给父作用域
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
    else()
        message(WARNING "Auto-Config: Path variable ${TARGET_VAR_NAME} is not defined. Please check your config.")
    endif()
endfunction()
