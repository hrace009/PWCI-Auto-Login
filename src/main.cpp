#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication::setWindowIcon(QIcon(":/images/resources/pwicon.ico"));
    QCoreApplication::setOrganizationName("PSDevHub");
    QCoreApplication::setApplicationName("PW Classic Indonesia Auto Login");
    MainWindow w;
    w.show();
    return a.exec();
}
