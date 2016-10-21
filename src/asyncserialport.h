#ifndef ASYNCSERIALPORT_H
#define ASYNCSERIALPORT_H

#include <QObject>
#include <QProcess>
#include <QSerialPort>
#include <QTimer>

class AsyncPort : public QObject
{
    Q_OBJECT

public:
    enum Status
    {
        Offline,
        Opening,
        Online,
        Disconnected,
        Error
    };
    Q_ENUM(Status)

    explicit AsyncPort(QObject *parent = 0);

    static QString convertStatusToQString(AsyncPort::Status st);

signals:
    void statusChanged(AsyncPort::Status st, const QString &pn, qint32 br);
    void dataReceived(QByteArray data, bool insertCR);

public slots:
    void initialize();
    void openSerialPort(const QString &pn, qint32 br);
    void openLocalShell();
    void closePort(Status st = Offline);
    void sendData(QByteArray data);

private slots:
    void readPort();
    void checkSerialPort();
    void serialPortError(QSerialPort::SerialPortError serialPortError);
    void startedLocalShell();
    void errorLocalShell(QProcess::ProcessError);
    void finishedLocalShell(int, QProcess::ExitStatus);
    void stateChangedLocalShell(QProcess::ProcessState state);

private:
    void updateStatus(Status st);
    const QString portName();
    qint32 baudRate();

    Status m_status;
    QTimer *m_serialConnectionCheckTimer;
    QSerialPort *m_serialPort;
    QProcess *m_localShell;
    QIODevice *m_port;
};

#endif // ASYNCSERIALPORT_H
