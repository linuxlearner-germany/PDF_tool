#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PDF Tool"));
    QApplication::setOrganizationName(QStringLiteral("PDF Tool"));

    MainWindow window;
    window.show();

    return QApplication::exec();
}
