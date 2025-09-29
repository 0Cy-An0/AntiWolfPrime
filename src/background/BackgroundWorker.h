#pragma once

#include <QThread>
#include <QTimer>
#include <atomic>
#include <QObject>
#include <QString>

class MainWindow;

class BackgroundWorker : public QObject {
    Q_OBJECT
public:
    explicit BackgroundWorker(MainWindow* mainWindow);

public slots:
    void startWork();
    void stopWork();

private slots:
    void backgroundWork();

private:
    //bool checkMissionFinished();
    void triggerUpdate();

    std::atomic<bool> running;
    int timersElapsed = 0;
    std::streampos lastReadPos = 0;

    MainWindow* mainWindow;
    QTimer* timer = nullptr;
};
