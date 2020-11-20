/****************************************************************************
 *
 * @author Dilshod Mukhtarov <dilshodm(at)gmail.com>
 * Sep 2018
 *
 ****************************************************************************/

#ifndef DEVICECOMMUNICATION_H
#define DEVICECOMMUNICATION_H

#include <QObject>
#include <QSerialPort>
#include <QVector>
#include <QRegularExpression>

enum {
    NUM_OF_PINS = 8,
    MIN_Y = 0,
    MAX_Y = 255,    // Don't change this, because original application is hard-coded
                    // to use 8-bit adc values all over the code
    SERIAL_WAIT_TIMEOUT = 100, // in milliseconds

    MAX_ERROR_IN_READ_DATA = 10,
};

class QSerialPort;
class QSerialPortInfo;

/*
 * DeviceCommunication class is responsible for communication wtih Arduino device
 * It replaces legacy hid_stuff class which was intended to use with Microchip PIC
 */
class DeviceCommunication : public QObject
{
    Q_OBJECT
private:
    enum class GetMode
    {
        Unknown,
        GetAE,
        GetTR,
        Settings,
        Command,
    };

public:
    explicit DeviceCommunication(QObject *parent = nullptr);
    virtual ~DeviceCommunication() override;

    void sendInitData();
    void sendCommand(const QByteArray &command);

    void readAeData();
    void readTrData();

    void parseAe(QByteArray &buffer);
    void parseTr(QByteArray &buffer);

signals:
    void dataAeReady(const QStringList &);
    void dataTrReady(const QStringList &);

    void isReadingDataChanged(bool);

    void readFromSettings(const QByteArray &);
    void readFromCommand(const QByteArray &);

private slots:
    void onReadyRead();

private:
    void connectDevice();
    void disconnectDevice();
    bool isLegitimateDevice(const QSerialPortInfo &portInfo) const;
    bool openAndCheckDevice(const QSerialPortInfo &portInfo);

    void setIsReadingData(bool isReadingData);

private:
    QSerialPort  m_serialPort;
    GetMode m_getMode = GetMode::Unknown;
    QStringList m_dataList;
    QString m_incompleteData;

    int m_linesToRead;
    bool m_isReadingData = false;
};

#endif // DEVICECOMMUNICATION_H
