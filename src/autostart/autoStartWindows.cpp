#ifdef WINDOWS_PLATFORM

#include "autostart.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>

bool setAutoStart(bool enable) {
    QString appName = QCoreApplication::applicationName();
    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);

    if (enable) {
        settings.setValue(appName, "\"" + appPath + "\"");
    } else {
        settings.remove(appName);
    }

    return true; //add error checking if needed
}

#endif // WINDOWS_PLATFORM
