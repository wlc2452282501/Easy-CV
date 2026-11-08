#include <QApplication>

#include "app/mainwindow.h"

// The application object owns the event loop; MainWindow owns all page widgets.
int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Qt + OpenCV Visual Platform"));
    application.setApplicationVersion(QStringLiteral("0.1.0"));

    MainWindow window;
    window.show();
    return application.exec();
}
