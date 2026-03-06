#include "EyAbstractWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

EyAbstractWidget::EyAbstractWidget(QWidget *parent)
    : QWidget(parent)
{
    // 注意：不要在父类构造函数中调用虚函数 setupUi()，
    // 因为此时子类尚未构造，虚函数表未建立，会导致调用错误。
    // 必须在子类的构造函数中调用 setupUi()。
}

void EyAbstractWidget::setupUi()
{
    initItems();
    initLayout();
    initConnections();
    initWidget();
}