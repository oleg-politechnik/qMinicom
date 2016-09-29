#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "preferencesdialog.h"
#include "searchhighlighter.h"

#include <QMainWindow>
#include <QSerialPort>
#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    const QString currentPortName();
    const QFont &logWidgetFont();

public slots:
    void openSerialDevice(const QString &portName, qint32 baudRate);
    void setLogWidgetSettings(const QFont &font, const QPalette &palette, int tabStopWidthPixels);

protected:
    void keyPressEvent(QKeyEvent* event);

private slots:
    void readPort();
    void updatePortStatus(bool isOpen);
    void portAboutToClose();
    void customLogWidgetContextMenuRequested(const QPoint &pos);
    void setFindWidgetVisible(bool visible);
    void updateSearch();

private:
    Ui::MainWindow *ui;

    PreferencesDialog *dlgPrefs;

    QSerialPort *m_port;

    SearchHighlighter *m_highlighter;

    void writeSettings();
    void readSettings();
};

#endif // MAINWINDOW_H
