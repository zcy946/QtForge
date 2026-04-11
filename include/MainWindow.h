/**
 * @file MainWindow.h
 * @author xialichen (xlc946@qq.com)
 * @brief CMake构建的Qt项目示例模板
 * @version 0.1
 * @date 2026-03-06
 *
 * @copyright Copyright (c) 2026  广州二研科技有限公司
 *
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "XlcAbstractWidget.h"
#include <QPaintEvent>

class MainWindow : public XlcAbstractWidget
{
public:
    explicit MainWindow(XlcAbstractWidget *parent = nullptr);
    virtual ~MainWindow();

protected:
    void initItems() override;
    void initLayout() override;
    void initConnections() override;
    void initWidget() override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // MAINWINDOW_H