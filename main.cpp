#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    //将高DPI缩放因子向下舍入到最接近的整数值。在高分辨率屏幕上的显示效果，使其更加清晰和准确。
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
#endif
    // 显示主窗口
    MainWindow w;
    w.show();
    
    return a.exec();
}
