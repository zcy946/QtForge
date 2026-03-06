/**
 * @file EyAbstractWidget.h
 * @author xialichen (xlc946@qq.com)
 * @brief 所有继承自`QWidget`的类都应该改为继承该类，进行统一初始化
 * @version 0.1
 * @date 2025-12-03
 *
 * @copyright Copyright (c) 2025  广州二研科技有限公司
 *
 */

#ifndef ABSTRACTWIDGET_H
#define ABSTRACTWIDGET_H

#include <QWidget>
class QHBoxLayout;
class QVBoxLayout;

/**
 * @brief 规范化 Qt 控件初始化流程的抽象基类
 *
 * @note
 * 所有自定义的 Widget 都应继承此类，并必须实现四个标准初始化步骤。
 * 在子类的构造函数中，请务必显式调用 setupUi() 以启动初始化流程。
 */
class EyAbstractWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EyAbstractWidget(QWidget *parent = nullptr);
    virtual ~EyAbstractWidget() = default;

protected:
    /**
     * @brief 模板方法：标准的 UI 初始化流程
     *
     * 请在子类的构造函数中直接调用此函数。
     * 它将严格按照：实例化 -> 布局 -> 连接 -> 配置 的顺序执行。
     */
    void setupUi();

    /**
     * @brief 第一步：实例化子控件与成员变量
     *
     * @note
     * 1. 使用 new 操作符实例化所有的成员变量指针（如 QLabel*, QPushButton* 等）。
     * 2. 设置控件的基本属性（如 objectName, fixedWidth 等不依赖布局的属性）。
     * 3. 初始化非 UI 的成员对象（如 Model, Timer 等）。
     * 4. 此时禁止将控件添加到布局中。
     */
    virtual void initItems() = 0;

    /**
     * @brief 第二步：组装布局
     *
     * @note
     * 1. 创建布局管理器（QVBoxLayout, QGridLayout 等）。
     * 2. 使用 addWidget/addLayout 将第一步创建的控件加入布局。
     * 3. 设置布局的属性（Margin, Spacing, Alignment）。
     * 4. 调用 setLayout() 将主布局应用到当前窗口。
     */
    virtual void initLayout() = 0;

    /**
     * @brief 第三步：建立信号与槽的连接
     *
     * @note
     * 1. 连接内部控件之间的交互（如 Button clicked -> StackedWidget setCurrentIndex）。
     * 2. 连接控件与当前类的业务逻辑槽函数。
     * 3. 可以在此使用 Lambda 表达式处理简单的逻辑。
     * 建议：将连接逻辑与布局逻辑分离，便于后续排查交互 bug。
     */
    virtual void initConnections() = 0;

    /**
     * @brief 第四步：初始化窗口自身属性与默认数据
     *
     * @note
     * 1. 设置窗口层级的属性（setWindowTitle, setWindowIcon, setStyleSheet）。
     * 2. 加载默认数据或从配置文件/数据库读取初始状态。
     * 3. 根据业务需求刷新一次 UI 显示（如 updateUI()）。
     * 4. 处理窗口刚显示时需要的焦点设置等。
     */
    virtual void initWidget() = 0;
};

#endif // ABSTRACTWIDGET_H

#if 0
// 使用实例
#include "EyAbstractWidget.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>

class MyCustomPanel : public EyAbstractWidget
{
    Q_OBJECT
public:
    explicit MyCustomPanel(QWidget *parent = nullptr) : EyAbstractWidget(parent)
    {
        // 核心：在构造函数最后调用标准化流程
        setupUi();
    }

private:
    // 成员变量指针
    QLabel *m_labelTitle;
    QPushButton *m_pushButtonConfirm;
    QCheckBox *m_checkBoxDisable;

protected:
    // 1. 初始化对象
    void initItems() override
    {
        m_labelTitle = new QLabel("标题", this);
        m_labelTitle->setObjectName("lblTitle");
        m_pushButtonConfirm = new QPushButton("确认", this);
        m_checkBoxDisable = new QCheckBox("禁用", this);
    }

    // 2. 布局排版
    void initLayout() override
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->addWidget(m_labelTitle);
        mainLayout->addStretch(); // 增加弹簧
        mainLayout->addWidget(m_pushButtonConfirm);
        mainLayout->addWidget(m_checkBoxDisable);
    }

    // 3. 信号连接
    void initConnections() override
    {
        connect(m_pushButtonConfirm, &QPushButton::clicked, this,
                [this]()
                {
                    QMessageBox::information(this, "提示", "按钮被点击了");
                });
        connect(m_checkBoxDisable, &QCheckBox::stateChanged, this,
                [this](int state)
                {
                    QMessageBox::information(this, "提示", state == 0 ? "禁用" : "启用");
                });
    }

    // 4. 窗口配置
    void initWidget() override
    {
        // 设置窗口本身的属性
        setWindowTitle("我的自定义面板");
        resize(400, 300);

        // 设置(需要触发信号控件的)初始数据状态
        m_checkBoxDisable->setCheckState(Qt::Unchecked);
    }
};

#endif