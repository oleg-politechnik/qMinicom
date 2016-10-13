#include "asyncserialport.h"

#include <QDebug>
#include <QMetaEnum>
#include <QSerialPortInfo>

AsyncSerialPort::AsyncSerialPort(QObject *parent) : QSerialPort(parent)
{
}

QString AsyncSerialPort::convertStatusToQString(AsyncSerialPort::Status st)
{
    //
    // (c) http://stackoverflow.com/a/34281989/6895308
    //

    const QMetaObject &mo = AsyncSerialPort::staticMetaObject;
    int index = mo.indexOfEnumerator("Status");
    QMetaEnum metaEnum = mo.enumerator(index);
    return metaEnum.valueToKey(st);
}

void AsyncSerialPort::initialize()
{
    m_connectionCheckTimer = new QTimer(this);
    connect(m_connectionCheckTimer, SIGNAL(timeout()), this, SLOT(checkPort()));

    updateStatus(Offline);
}

void AsyncSerialPort::openPort(const QString &pn, qint32 br)
{
    if (pn.isEmpty())
    {
        return;
    }

    if (isOpen())
    {
        if (portName() != pn)
        {
            close();
            qDebug() << "port" << portName() << "closed";
        }
        else if (baudRate() != br)
        {
            setBaudRate(br);
            qDebug() << "port" << portName() << "@" << baudRate() << "opened";

            updateStatus(Online);
        }
    }

    if (!isOpen())
    {
        setPortName(pn);

        if (!open(QSerialPort::ReadWrite))
        {
            qDebug() << "ERROR Can't open" << pn;

            updateStatus(Error);
        }
        else
        {
            setParity(QSerialPort::NoParity); // todo if
            setDataBits(QSerialPort::Data8); // todo if
            setParity(QSerialPort::NoParity); // todo if
            setStopBits(QSerialPort::OneStop); // todo if
            setFlowControl(QSerialPort::NoFlowControl); // todo if

            setBaudRate(br);

            clear();

            qDebug() << "port" << portName() << "@" << baudRate() << "opened";

            connect(this, SIGNAL(readyRead()), SLOT(readPort()));

            updateStatus(Online);

            m_connectionCheckTimer->start(1000);
        }
    }
}

void AsyncSerialPort::closePort(AsyncSerialPort::Status st)
{
    if (isOpen())
    {
        close();
        qDebug() << "port" << portName() << "closed";
        disconnect(this, SIGNAL(readyRead()), this, SLOT(readPort()));
    }

    m_connectionCheckTimer->stop();

    clearError();

    updateStatus(st);
}

void AsyncSerialPort::sendData(QByteArray data)
{
    checkPort();

    if (!isOpen())
    {
        qDebug() << "Warning:" << __FUNCTION__ << ": port is not opened";
    }
    else
    {
        write(data);
    }
}

void AsyncSerialPort::readPort()
{
    QByteArray data = readAll();

    emit dataReceived(data);
}

void AsyncSerialPort::checkPort()
{
    if (isOpen())
    {
        QSerialPortInfo info(*this);
        if (!info.isValid())
        {
            qDebug() << "Warning: port" << portName() << "is invalid, closing...";
            closePort(Disconnected);
        }
    }
}

void AsyncSerialPort::serialPortError(QSerialPort::SerialPortError serialPortError)
{
    qDebug() << "Serial port error:" << serialPortError << ", closing...";
    closePort(Error);
}

void AsyncSerialPort::updateStatus(AsyncSerialPort::Status st)
{
    m_status = st;
    emit statusChanged(m_status, portName(), baudRate());
}
