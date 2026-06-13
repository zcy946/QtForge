# ================================================================================
# QtForge 日志控件示例程序
# ================================================================================
set(QTFORGE_LOG_WIDGET_EXAMPLE_TARGET QtForgeLogWidgetExample)

add_executable(${QTFORGE_LOG_WIDGET_EXAMPLE_TARGET}
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

set_target_properties(${QTFORGE_LOG_WIDGET_EXAMPLE_TARGET} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    WIN32_EXECUTABLE $<$<CONFIG:Release>:TRUE>
)

target_include_directories(${QTFORGE_LOG_WIDGET_EXAMPLE_TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(${QTFORGE_LOG_WIDGET_EXAMPLE_TARGET} PRIVATE
    XlcLogger
    XlcLoggerQtFormatter
    XlcLogWidget
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)

if(WIN32)
    add_custom_command(TARGET ${QTFORGE_LOG_WIDGET_EXAMPLE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:XlcLogger>
        $<TARGET_FILE_DIR:${QTFORGE_LOG_WIDGET_EXAMPLE_TARGET}>
        COMMENT "Copying XlcLogger runtime library to executable directory"
    )
endif()
