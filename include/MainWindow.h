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

class QPushButton;
class QPlainTextEdit;
class XlcLogWidget;
class MainWindow : public XlcAbstractWidget
{
public:
    explicit MainWindow(XlcAbstractWidget *parent = nullptr);
    virtual ~MainWindow();

protected:
    void initItems() override;
    void initLayout() override;
    void initConnections() override;

private:
    QPushButton *m_pushButtonAddTrace;
    QPushButton *m_pushButtonAddDebug;
    QPushButton *m_pushButtonAddInfo;
    QPushButton *m_pushButtonAddWarn;
    QPushButton *m_pushButtonAddError;
    QPushButton *m_pushButtonAddCritical;
    QPlainTextEdit *m_plainTextEdit;
    XlcLogWidget *m_xlcLogWidget;
};

#endif // MAINWINDOW_H