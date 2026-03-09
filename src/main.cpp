#include <QApplication>
#include "MainWindow.h"
#include "EyLogger.hpp"

int logger = []() -> int
{
    EyLogger::init();
    return 0;
}();

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MainWindow w;
    w.resize(800, 600);
    w.show();

    app.exec();
    return 0;
}