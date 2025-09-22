#include "BackgroundWorker.h"
#include "window/mainwindow.h"
#include <QTimer>
#include <QMetaObject>
#include <fstream>
#include <EELogParser/logParser.h>
#include "FileAccess/FileAccess.h"

BackgroundWorker::BackgroundWorker(MainWindow* mw)
    : QObject(nullptr), running(true), mainWindow(mw), timer(new QTimer(this)) {
    timer->setInterval(60000); //60000 ms = 1 minute
    connect(timer, &QTimer::timeout, this, &BackgroundWorker::backgroundWork);
}

void BackgroundWorker::startWork() {
    running = true;
    timersElapsed = 0;
    timer->start();
}

void BackgroundWorker::stopWork() {
    running = false;
    timer->stop();
}

void BackgroundWorker::backgroundWork() {
    if (!running) return;

    Settings settings = MainWindow::getWindowSettings();

    bool shouldUpdate = false;

    if (settings.autoSync) {
        ++timersElapsed;
        if (timersElapsed >= settings.syncTime) {
            LogThis("triggering Sync: timer");
            timersElapsed = 0;
            shouldUpdate = true;
        }
    }

    if (settings.syncOnMissionFinish && checkMissionFinished()) {
        shouldUpdate = true;
    }

    if (shouldUpdate) {
        triggerUpdate();
    }
}

bool BackgroundWorker::checkMissionFinished() {
    static bool initialized = false;
    bool found = false;

    const std::string EElogPath = findLog();
    if (EElogPath.empty()) return false;

    std::ifstream file(EElogPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;

    // Check for truncation (file restart)
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    if (lastReadPos > fileSize) {
        // Log file was reset
        LogThis("EELog was cleard");
        lastReadPos = 0;
        initialized = false;
    }

    // First time setup: Scan whole file to prime position
    if (!initialized) {
        //jump to end
        int pos = fileSize;

        while (pos > 0) {
            file.seekg(--pos);
            if (file.peek() == '\n') {
                file.get();
                break;
            }
        }
        lastReadPos = pos;
        initialized = true;
        return false; //its fine if this misses a mission
    }

    // Normal tailing from last position
    file.clear();
    file.seekg(lastReadPos);
    std::string line;
    std::streampos currentPos = lastReadPos;

    while (true) {
        if (!std::getline(file, line)) {
            // Couldn't read a full line — likely EOF or partial write
            break;
        }

        std::streampos afterLinePos = file.tellg(); // end of line

        // Check if line was complete: if not, don't update lastReadPos
        if (file.eof() && (line.empty() || (line.back() != '\n' && line.back() != '\r'))) {
            break;
        }

        if (line.find("EndOfMatch.lua: ReturnedToShip") != std::string::npos) {
            found = true;
        }

        currentPos = afterLinePos;
    }

    lastReadPos = currentPos;

    if (found) {
        LogThis("triggering Sync: mission finish");
        return true;
    }
    return false;
}


void BackgroundWorker::triggerUpdate() {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, [this]() {
            mainWindow->updatePlayerData();
        }, Qt::QueuedConnection);
    }
}