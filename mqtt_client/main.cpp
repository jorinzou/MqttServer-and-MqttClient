#include "mainwindow.h"
#include <QApplication>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QList>
#include <QDebug>
#include <QString>
#include "mqtt/qmqtt.h"
#include <time.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.MainWinIinit();
    w.CommonWinInit();
    w.show();

    return a.exec();
}
