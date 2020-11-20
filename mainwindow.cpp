#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPushButton>
#include <QElapsedTimer>
#include <QDebug>
#include <QStringList>
#include <QDateTime>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->startButton, &QPushButton::pressed, this, &MainWindow::getAeData);
    connect(ui->getTrButton, &QPushButton::pressed, this, &MainWindow::getTrData);

    connect(&m_device, &DeviceCommunication::dataAeReady, this, &MainWindow::onReceivedAeData);
    connect(&m_device, &DeviceCommunication::dataTrReady, this, &MainWindow::onReceivedTrData);
    connect(&m_device, &DeviceCommunication::readFromSettings, this, &MainWindow::onReadFromSettings);
    connect(&m_device, &DeviceCommunication::readFromCommand, this, &MainWindow::onReadFromCommand);

    connect(&m_device, &DeviceCommunication::isReadingDataChanged,
            this, [this] (bool isReadingData)
    {
        ui->startButton->setDisabled(isReadingData);
    });


    m_device.sendInitData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getData()
{
    m_logFile.setFileName("log_" + QString::number(QDateTime::currentSecsSinceEpoch()) + ".txt");
    m_logFile.open(QIODevice::ReadWrite | QIODevice::Text);

    ui->plainTextEdit->appendPlainText("Started reading from serial port\n");

    m_elapsedTimer.start();
    m_device.readAeData();
}

void MainWindow::getAeData()
{
    qDebug() << __func__;
    ui->plainTextEdit->appendPlainText("Sending get_ae_data command...");
    m_device.sendCommand("get_ae_data\n");
}

void MainWindow::getTrData()
{
    qDebug() << __func__;
    ui->plainTextEdit->appendPlainText("Sending get_tr_data command...");
    m_device.sendCommand("get_tr_data\n");
}

void MainWindow::onReceivedAeData(const QStringList &dataList)
{
    ++m_nReadDataCalled;

    QString output = QDateTime::currentDateTime().toString("[dd.MM.yyyy - hh:mm:ss.z]\n")
            + dataList.join('\n') + "\n\n";
    QTextStream logStream(&m_logFile);
    logStream << output;

    bool hitFound = false;
    for (const auto &line : dataList) {
        if (line.front() == 'H') {
            if (!hitFound) {
                hitFound = true;
            }

            ++m_nHits;
        }
    }

    ui->plainTextEdit->appendPlainText("hits registered: " + QString::number(m_nHits));
    m_nHits = 0;

    if (hitFound) {
        m_device.readTrData();
    }
}

void MainWindow::onReceivedTrData(const QStringList &dataList)
{
    QString output = QDateTime::currentDateTime().toString("[dd.MM.yyyy - hh:mm:ss.z]\n")
            + dataList.join('\n') + "\n\n";
    QTextStream logStream(&m_logFile);
    logStream << output;

    if (m_elapsedTimer.elapsed() < 1000 * 5) {
        m_device.readAeData();
    } else {
        m_logFile.close();

        ui->plainTextEdit->appendPlainText(QString("Finished reading. Number of iterations: ")
                                           + QString::number(m_nReadDataCalled) + "\n\n");

        m_nReadDataCalled = 0;
    }
}

void MainWindow::onReadFromSettings(const QByteArray &buffer)
{
    QFile logFile("settings-reply_" + QString::number(QDateTime::currentSecsSinceEpoch()) + ".txt");
    logFile.open(QIODevice::ReadWrite | QIODevice::Text);
    QTextStream logStream(&logFile);
    logStream << buffer;
    logStream.flush();
    logFile.close();
}

void MainWindow::onReadFromCommand(const QByteArray &buffer)
{
    QFile logFile("command-reply_" + QString::number(QDateTime::currentSecsSinceEpoch()) + ".txt");
    logFile.open(QIODevice::ReadWrite | QIODevice::Text);
    QTextStream logStream(&logFile);
    logStream << buffer;
    logStream.flush();
    logFile.close();

    ui->plainTextEdit->appendPlainText(QString("get ae data received %1 bytes").arg(buffer.size()));
}
