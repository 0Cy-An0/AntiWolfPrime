#ifdef LINUX_PLATFORM

#include "Autostart.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

bool setAutoStart(bool enable) {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString autostartDir = configDir + "/autostart";
    QDir().mkpath(autostartDir);

    QString desktopFilePath = autostartDir + "/" + QCoreApplication::applicationName() + ".desktop";
    QString execPath = QCoreApplication::applicationFilePath();

    if (enable) {
        QFile file(desktopFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Type=Application\n"
                << "Exec=" << execPath << "\n"
                << "Hidden=false\n"
                << "NoDisplay=false\n"
                << "X-GNOME-Autostart-enabled=true\n"
                << "Name=" << QCoreApplication::applicationName() << "\n";
            file.close();
            return true;
        } else {
            return false;
        }
    } else {
        return QFile::remove(desktopFilePath);
    }
}

#endif // LINUX_PLATFORM
