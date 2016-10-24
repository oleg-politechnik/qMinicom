#include "asyncserialport.h"

#include <QDebug>
#include <QMetaEnum>
#include <QSerialPortInfo>

AsyncPort::AsyncPort(QObject *parent) :
    QObject(parent),
    m_serialConnectionCheckTimer(Q_NULLPTR),
    m_serialPort(Q_NULLPTR),
    m_localShell(Q_NULLPTR),
    m_port(Q_NULLPTR)
{
}

QString AsyncPort::convertStatusToQString(AsyncPort::Status st)
{
    //
    // (c) http://stackoverflow.com/a/34281989/6895308
    //

    const QMetaObject &mo = AsyncPort::staticMetaObject;
    int index = mo.indexOfEnumerator("Status");
    QMetaEnum metaEnum = mo.enumerator(index);
    return metaEnum.valueToKey(st);
}

void AsyncPort::initialize()
{
    m_serialConnectionCheckTimer = new QTimer(this);
    connect(m_serialConnectionCheckTimer, SIGNAL(timeout()), this, SLOT(checkSerialPort()));

    m_serialPort = new QSerialPort(this);

    m_localShell = new QProcess(this);
    QProcessEnvironment env = m_localShell->processEnvironment();
    env.insert("TERM", "vt100");
    m_localShell->setProcessEnvironment(env);
    m_localShell->setProcessChannelMode(QProcess::MergedChannels);

    updateStatus(Offline);
}

void AsyncPort::openSerialPort(const QString &pn, qint32 br)
{
    if (pn.isEmpty())
    {
        return;
    }

    if (m_port == m_localShell)
    {
        closePort(); // shut down local shell
        Q_ASSERT(m_port == Q_NULLPTR);
    }

    if (m_port != m_serialPort)
    {
        m_port = m_serialPort;
    }

    if (m_serialPort->isOpen())
    {
        if (m_serialPort->portName() != pn)
        {
            m_serialPort->close();
            qDebug() << "port" << m_serialPort->portName() << "closed";
        }
        else if (m_serialPort->baudRate() != br)
        {
            m_serialPort->setBaudRate(br);
            qDebug() << "port" << m_serialPort->portName() << "@" << m_serialPort->baudRate() << "opened";

            updateStatus(Online);
        }
    }

    if (!m_serialPort->isOpen())
    {
        m_serialPort->setPortName(pn);

        bool portInited = (m_serialPort->setParity(QSerialPort::NoParity) &&
                           m_serialPort->setDataBits(QSerialPort::Data8) &&
                           m_serialPort->setStopBits(QSerialPort::OneStop) &&
                           m_serialPort->setFlowControl(QSerialPort::NoFlowControl) &&
                           m_serialPort->setBaudRate(br));

        if (!m_serialPort->open(QSerialPort::ReadWrite))
        {
            qDebug() << "ERROR Can't open" << pn;

            updateStatus(Error);
        }
        else
        {
            if (!portInited)
            {
                portInited = (m_serialPort->setParity(QSerialPort::NoParity) &&
                              m_serialPort->setDataBits(QSerialPort::Data8) &&
                              m_serialPort->setStopBits(QSerialPort::OneStop) &&
                              m_serialPort->setFlowControl(QSerialPort::NoFlowControl) &&
                              m_serialPort->setBaudRate(br));
            }

            if (!portInited)
            {
                qDebug() << "ERROR Can't init" << pn;
                updateStatus(Error);
            }
            else
            {
                m_serialPort->clear();
                m_serialPort->clearError();

                qDebug() << "port" << m_serialPort->portName() << "@" << m_serialPort->baudRate() << "opened";

                connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(readPort()));

                updateStatus(Online);

                m_serialConnectionCheckTimer->start(1000);

                connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(serialPortError(QSerialPort::SerialPortError)));
            }
        }
    }
}

void AsyncPort::openLocalShell()
{
    if (m_port == m_serialPort)
    {
        closePort(); // shut down serial port
        Q_ASSERT(m_port == Q_NULLPTR);
    }

    if (m_port != m_localShell)
    {
        m_port = m_localShell;
    }

    connect(m_localShell, SIGNAL(readyRead()), this, SLOT(readPort()));

    connect(m_localShell, SIGNAL(started()), this, SLOT(startedLocalShell()));
    connect(m_localShell, SIGNAL(error(QProcess::ProcessError)), this, SLOT(errorLocalShell(QProcess::ProcessError)));
    connect(m_localShell, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finishedLocalShell(int,QProcess::ExitStatus)));
    connect(m_localShell, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChangedLocalShell(QProcess::ProcessState)));

    m_localShell->start("sh", QStringList("-i"), QIODevice::ReadWrite);

    updateStatus(Online);
}

void AsyncPort::closePort(AsyncPort::Status st)
{
    Q_ASSERT(st != Online);

    if (m_port)
    {
        disconnect(m_port, SIGNAL(readyRead()), this, SLOT(readPort()));
    }

    if (m_port == m_serialPort)
    {
        if (m_serialPort->isOpen())
        {
            m_serialPort->close();
            qDebug() << "port" << m_serialPort->portName() << "closed";
        }
        m_serialConnectionCheckTimer->stop();
        m_serialPort->clearError();
        connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(serialPortError(QSerialPort::SerialPortError)));
    }
    else if (m_port == m_localShell)
    {
        if (m_localShell->isOpen())
        {
            disconnect(m_localShell, SIGNAL(started()), this, SLOT(startedLocalShell()));
            disconnect(m_localShell, SIGNAL(error(QProcess::ProcessError)), this, SLOT(errorLocalShell(QProcess::ProcessError)));
            disconnect(m_localShell, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finishedLocalShell(int,QProcess::ExitStatus)));
            disconnect(m_localShell, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChangedLocalShell(QProcess::ProcessState)));

            m_localShell->close();
            qDebug() << "local shell closed"; // XXX wait for finish?
        }
    }

    m_port = Q_NULLPTR;
}

void AsyncPort::sendData(QByteArray data)
{
    //qDebug() << __FUNCTION__;

    if (!m_port)
    {
        qDebug() << "Warning:" << __FUNCTION__ << ": no port is opened";
        return;
    }

    if (!m_port->isOpen())
    {
        qDebug() << "Warning:" << __FUNCTION__ << ": port" << portName() << "is not opened";
    }
    else
    {
        m_port->write(data);
    }
}

void AsyncPort::readPort()
{
    //qDebug() << __FUNCTION__;

    if (!m_port)
    {
        qDebug() << "Warning:" << __FUNCTION__ << ": no port is opened";
        return;
    }

    QByteArray data = m_port->readAll();
    emit dataReceived(data, m_port == m_localShell);
}

void AsyncPort::checkSerialPort()
{
    Q_ASSERT(m_port == m_serialPort);

    if (m_serialPort->isOpen())
    {
        QSerialPortInfo info(*m_serialPort);
        if (!info.isValid())
        {
            qDebug() << "Warning: port" << m_serialPort->portName() << "is invalid, closing...";
            closePort(Disconnected);
        }
    }
}

void AsyncPort::serialPortError(QSerialPort::SerialPortError serialPortError)
{
    Q_ASSERT(m_port == m_serialPort);

    if (serialPortError != QSerialPort::NoError)
    {
        qDebug() << "Serial port error:" << serialPortError << ", closing...";
        closePort(Error);
    }
}


void AsyncPort::startedLocalShell()
{
    //qDebug() << __FUNCTION__;
}

void AsyncPort::errorLocalShell(QProcess::ProcessError e)
{
    qDebug() << __FUNCTION__ << e;
}

void AsyncPort::finishedLocalShell(int r, QProcess::ExitStatus es)
{
    qDebug() << __FUNCTION__ << r << es;
}

void AsyncPort::stateChangedLocalShell(QProcess::ProcessState state)
{
    switch (state)
    {
    case QProcess::NotRunning:
        updateStatus(Offline);
        break;

    case QProcess::Starting:
        updateStatus(Opening);
        break;

    case QProcess::Running:
        updateStatus(Online);
        qDebug() << "local shell opened";
        break;
    }
}


void AsyncPort::updateStatus(AsyncPort::Status st)
{
    m_status = st;
    emit statusChanged(m_status, portName(), baudRate());
}

const QString AsyncPort::portName()
{
    if (m_port == m_serialPort)
    {
        return m_serialPort->portName();
    }

    if (m_port == m_localShell)
    {
        return m_localShell->program();
    }

    return "";
}

qint32 AsyncPort::baudRate()
{
    if (m_port == m_serialPort)
    {
        return m_serialPort->baudRate();
    }

    return 0;
}
