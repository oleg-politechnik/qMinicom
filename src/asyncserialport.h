#ifndef ASYNCSERIALPORT_H
#define ASYNCSERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>

class AsyncSerialPort : public QSerialPort
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

    explicit AsyncSerialPort(QObject *parent = 0);

    static QString convertStatusToQString(AsyncSerialPort::Status st);

signals:
    void statusChanged(AsyncSerialPort::Status st, const QString &pn, qint32 br);
    void dataReceived(QByteArray data);

public slots:
    void initialize();
    void openPort(const QString &pn, qint32 br);
    void closePort(Status st = Offline);
    void sendData(QByteArray data);

private slots:
    void readPort();
    void checkPort();
    void serialPortError(QSerialPort::SerialPortError serialPortError);

private:
    void updateStatus(Status st);

    Status m_status;
    QTimer *m_connectionCheckTimer;
};

#endif // ASYNCSERIALPORT_H
