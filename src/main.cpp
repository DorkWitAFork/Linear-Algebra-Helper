#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("Linear Algebra Helper");
    application.setApplicationVersion("1.0.0");
    application.setOrganizationName("Linear Algebra Helper");
    application.setStyle(QStyleFactory::create("Fusion"));
    la::MainWindow window;
    window.show();
    return application.exec();
}
