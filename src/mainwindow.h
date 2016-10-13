#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "asyncserialport.h"
#include "preferencesdialog.h"

#include <QMainWindow>
#include <QSettings>
#include <QThread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    const QString &currentPortName();
    const QFont &logWidgetFont();

signals:
    void openPort(const QString &pn, qint32 br);
    void closePort();

public slots:
    void setLogWidgetSettings(const QFont &font, const QPalette &palette, int tabStopWidthPixels);

protected:
    void keyPressEvent(QKeyEvent* event);

private slots:
    void updatePortStatus(AsyncSerialPort::Status st, const QString &pn, qint32 br);
    void customLogWidgetContextMenuRequested(const QPoint &pos);
    void setFindWidgetVisible(bool visible);
    void updateSearch();

private:
    void writeSettings();
    void readSettings();

    Ui::MainWindow *ui;
    PreferencesDialog *m_dlgPrefs;
    QThread m_asyncSerialPortThread;
    QString m_portName;
};

#endif // MAINWINDOW_H
