#include <fstream>

#include "mainwindow.h"

#include <iostream>
#include <QApplication>
#include <QPalette>


int main(int argc, char *argv[])
{
    //Clearing the Log
    std::ofstream logFile("../data/Log/Log.txt", std::ofstream::trunc);

    QApplication a(argc, argv);

    /*QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "AntiWolfPrimeWindow_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }*/

    a.setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(40, 40, 40));
    a.setPalette(darkPalette);

    MainWindow w;
    w.setWindowTitle("AntiWolfPrime");
    w.setStyleSheet("background-color: #282828;");
    w.show();
    return a.exec();
}
