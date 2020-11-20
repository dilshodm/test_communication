#pragma once

#include <QMainWindow>
#include <QElapsedTimer>
#include <QFile>
#include "devicecommunication.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void getData();
    void onReceivedAeData(const QStringList &dataList);
    void onReceivedTrData(const QStringList &dataList);
    void onReadFromSettings(const QByteArray &buffer);

private:
    Ui::MainWindow *ui;

    DeviceCommunication m_device;
    QElapsedTimer m_elapsedTimer;
    QFile m_logFile;

    int m_nReadDataCalled = 0;
    int m_nHits = 0;
};
