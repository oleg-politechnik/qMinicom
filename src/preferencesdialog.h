#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

class MainWindow;

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(MainWindow *parent = 0);
    ~PreferencesDialog();

public slots:
    void open();
    void accept();

private slots:
    void pickUpTextShadeValue(int val);
    void pickUpBgShadeValue(int val);
    void pickUpFont(const QString &name);
    void pickUpFontSize(int val);

private:
    void readSettings();
    void writeSettings();

    Ui::PreferencesDialog *ui;

    MainWindow *m_mainWindow;
    int m_textShade;
    int m_bgShade;
};

#endif // PREFERENCESDIALOG_H
