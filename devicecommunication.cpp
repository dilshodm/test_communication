/****************************************************************************
 *
 * @author Dilshod Mukhtarov <dilshodm(at)gmail.com>
 * Sep 2018
 *
 ****************************************************************************/

#include "devicecommunication.h"

#include <QSerialPortInfo>
#include <QVector>
#include <QPair>
#include <QDebug>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStringList>
#include <thread> // temp
#include <chrono> // temp

using namespace std::chrono; // temp

// Arduino Leonardo

// Keep here known list if devices
//  { VID,    PID }
const static QVector<QPair<uint16_t, uint16_t>> DEVICES_LIST = {
//    { 0x20a0, 0x4267 },    // Funkey board (Architectronics)
//    { 0x2341, 0x8036 },    // Arduino Leonardo { VID, PID }
//    { 0x2341, 0x0042 },    // Arduino/Genuino Mega or Mega 2560
    { 0x2291, 0x0110 },    // Vallen Systeme spotWave
    { 0x2341, 0x0043 },    // Arduino Uno
//    { 0x239A, 0x8014 },    // Adafruit Metro
//    { 0x239A, 0x800B },    // Adafruit Feather M0
//    { 0x239A, 0x801F },    // Adafruit Trinket M0
//    { 0x0D28, 0x0204 },    // MicroBit
};

#ifdef Q_OS_WIN
const static QString PORT_PREFIX("COM");    // Windows
#else
const static QString PORT_PREFIX("tty");    // Mac, Linux
#endif

DeviceCommunication::DeviceCommunication(QObject *parent)
    : QObject(parent)
{
    qDebug() << DEVICES_LIST;

    connect(&m_serialPort, &QSerialPort::readyRead, this, &DeviceCommunication::onReadyRead);
    connect(&m_serialPort, &QSerialPort::errorOccurred,
            this, [] (QSerialPort::SerialPortError error) { qDebug() << "Serial Error:" << error; });
    connectDevice();
}

DeviceCommunication::~DeviceCommunication()
{
    disconnectDevice();
}

void DeviceCommunication::sendInitData()
{
    m_getMode = GetMode::Settings;
    QByteArray command = "set_filter 80 350 8\n"
                         "set_acq thr 1000\n"
                         "set_acq ddt 400\n"
                         "set_acq tr_enabled 1\n"
                         "set_acq status_interval 1000\n"
                         "set_acq enabled 1\n"
            ;
    m_serialPort.write(command);
}

void DeviceCommunication::sendCommand(const QByteArray &command)
{
    m_getMode = GetMode::Command;
    m_serialPort.write(command);
}

void DeviceCommunication::connectDevice()
{
    bool isFoundDevice = false;
    for (const auto &portInfo : QSerialPortInfo::availablePorts()) {
        if (isLegitimateDevice(portInfo)) {
            m_serialPort.setPort(portInfo);
            if (openAndCheckDevice(portInfo)) {
                isFoundDevice = true;
                break;
            }
        }
    }

    if (!isFoundDevice) {
        disconnectDevice();
        return;
    }
}

void DeviceCommunication::disconnectDevice()
{
    m_serialPort.close();
}

bool DeviceCommunication::isLegitimateDevice(const QSerialPortInfo &portInfo) const
{
    bool isLegitimate = false;
    for (const auto &deviceInfo : DEVICES_LIST) {
        qDebug() << portInfo.vendorIdentifier() << portInfo.productIdentifier() << portInfo.portName();
        if (portInfo.vendorIdentifier() == deviceInfo.first && deviceInfo.second == portInfo.productIdentifier()
                && portInfo.portName().startsWith(PORT_PREFIX))
        {
            qDebug() << "Found port:" << portInfo.portName();
            isLegitimate = true;
            break;
        }
    }

    return isLegitimate;
}

bool DeviceCommunication::openAndCheckDevice(const QSerialPortInfo &portInfo)
{
    m_serialPort.setBaudRate(QSerialPort::Baud115200);

    m_serialPort.setParity(QSerialPort::NoParity);
    m_serialPort.setDataBits(QSerialPort::Data8);
    m_serialPort.setStopBits(QSerialPort::OneStop);

    m_serialPort.setFlowControl(QSerialPort::HardwareControl);

    if (!m_serialPort.open(QIODevice::ReadWrite)) {
        qDebug() << "Communicator open failed " << m_serialPort.errorString();
        disconnectDevice();
        return false;
    }

    return true;
}

void DeviceCommunication::readAeData()
{
    if (m_isReadingData) {
        return;
    }
    setIsReadingData(true);

    m_dataList.clear();
    m_getMode = GetMode::GetAE;
    m_linesToRead = 1;

    QByteArray command = "get_ae_data\n";
    m_serialPort.write(command);
}

void DeviceCommunication::readTrData()
{
    if (m_isReadingData) {
        return;
    }
    setIsReadingData(true);

    m_dataList.clear();
    m_getMode = GetMode::GetTR;
    m_linesToRead = 1;

    QByteArray command = "get_tr_data\n";
    m_serialPort.write(command);
}

void DeviceCommunication::parseAe(QByteArray &buffer)
{
    bool isComplete = buffer.back() == '\n';
    QStringList list = QString(buffer).split('\n');
    if (list.back().isEmpty()) {
        list.pop_back();
    }

    if (!m_incompleteData.isEmpty()) {
        list.front() = m_incompleteData + list.front();
        m_incompleteData.clear();
    }

    if (!isComplete) {
        m_incompleteData = list.back();
        list.pop_back();
    }

    if (m_dataList.empty() && !list.empty()) {
        m_linesToRead = list.front().toInt() + 1;
    }

    m_dataList.append(list);

    if (m_dataList.size() >= m_linesToRead) {
        setIsReadingData(false);
        emit dataAeReady(m_dataList);
    }
}

void DeviceCommunication::parseTr(QByteArray &buffer)
{
    bool isComplete = buffer.back() == '\n';
    QStringList list = QString(buffer).split('\n');
    if (list.back().isEmpty()) {
        list.pop_back();
    }

    if (!m_incompleteData.isEmpty()) {
        list.front() = m_incompleteData + list.front();
        m_incompleteData.clear();
    }

    if (!isComplete) {
        m_incompleteData = list.back();
        list.pop_back();
    }

    m_dataList.append(list);

    if (m_dataList.size() >= m_linesToRead) {
        static const QRegularExpression re("NS=(\\d+)");
        const auto &infoStr = m_dataList.at(m_linesToRead - 1);
        auto match = re.match(infoStr);
        if (match.hasMatch()) {
            auto nsString = match.captured(1);
            m_linesToRead += nsString.toInt() + 1;
        } else if (infoStr == "0") {
            setIsReadingData(false);
            emit dataTrReady(m_dataList);
        } else {
            qDebug() << "ERROR: unexpected line" << infoStr << m_linesToRead << "in buffer" << buffer;
        }
    }
}

void DeviceCommunication::onReadyRead()
{
    auto buffer = m_serialPort.readAll();
    qDebug() << "Received" << buffer;

    switch (m_getMode) {
    case GetMode::GetAE:
        parseAe(buffer);
        break;

    case GetMode::GetTR:
        parseTr(buffer);
        break;

    case GetMode::Settings:
        emit readFromSettings(buffer);
        break;

    case GetMode::Command:
        emit readFromCommand(buffer);
        break;

    default:
        qDebug() << "ERROR: Read from unkown mode";
        break;
    }
}

void DeviceCommunication::setIsReadingData(bool isReadingData)
{
    m_isReadingData = isReadingData;
    emit isReadingDataChanged(m_isReadingData);
}
