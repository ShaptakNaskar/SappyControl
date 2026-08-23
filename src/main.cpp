#include "mainwindow.h"

#include <QApplication>
#include <QFont>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Sappy's Controls"));
    QApplication::setApplicationDisplayName(QStringLiteral("Sappy's Controls"));
    QApplication::setOrganizationName(QStringLiteral("Sappy"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.sappy.SappyControls"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/sappy-controls.svg")));

    QFont font = application.font();
    font.setPointSize(10);
    application.setFont(font);

    MainWindow window;
    window.show();
    return application.exec();
}
