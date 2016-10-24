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

    const QFont &logWidgetFont();

signals:
    void closePort();
    void openSerialPort(const QString &pn, qint32 br);
    void openLocalShell();

public slots:
    void setLogWidgetSettings(const QFont &font, int tabStopWidthPixels);

protected:
    void keyPressEvent(QKeyEvent* event);

private slots:
    void updatePortStatus(AsyncPort::Status st, const QString &pn, qint32 br);
    void customLogWidgetContextMenuRequested(const QPoint &pos);
    void setFindWidgetVisible(bool visible);
    void showFindWidget(void);
    void updateSearch();
    void logWindowHorizontalBarRangeChanged(int min, int max);

private:
    void writeSettings();
    void readSettings();

    Ui::MainWindow *ui;
    PreferencesDialog *m_dlgPrefs;
    QThread m_asyncPortThread;
};

#endif // MAINWINDOW_H
