#include "mainwindow.h"
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QDir>
#include <QDebug>
#include <QSerialPortInfo>
#include <QPushButton>
#include <QColorDialog>

PreferencesDialog::PreferencesDialog(MainWindow *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    m_mainWindow = parent;

    ui->setupUi(this);

    ui->tabWidget->setCurrentIndex(0);

    ui->fontSizeSpinBox->setMinimum(9);
    ui->fontSizeSpinBox->setMaximum(72);

    connect(ui->fontComboBox, SIGNAL(activated(QString)), this, SLOT(pickUpFont(QString)));
    connect(ui->fontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(pickUpFontSize(int)));
    connect(ui->tabSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(pickUpTabSize(int)));

    ui->tabSizeSpinBox->setMinimum(1);

    // populate speed list

    QList<qint32> rates = QSerialPortInfo::standardBaudRates();
    if (!rates.contains(921600))
    {
        rates.append(921600); // custom rate: todo enable user to add new rates and store them in settings
    }

    std::sort(rates.begin(), rates.end());
    std::reverse(rates.begin(), rates.end()); // make faster ones on top

    foreach (qint32 br, rates)
    {
        ui->cmbSpeed->addItem(QString("%1").arg(br));
    }

    //

    connect(this, SIGNAL(openPort(QString,qint32)), m_mainWindow, SIGNAL(openPort(QString,qint32)));

    readSettings();
}

PreferencesDialog::~PreferencesDialog()
{
    writeSettings();

    delete ui;
}

void PreferencesDialog::open()
{
    // populate the device list

    ui->cmbDevice->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    foreach (QSerialPortInfo pi, ports)
    {
        ui->cmbDevice->addItem(pi.portName());
    }

    ui->cmbDevice->setCurrentIndex(ui->cmbDevice->findText(m_mainWindow->currentPortName()));

    QDialog::open();
}

void PreferencesDialog::accept()
{
    QDialog::accept();

    emit openPort(ui->cmbDevice->currentText(), ui->cmbSpeed->currentText().toUInt());

    m_mainWindow->setLogWidgetSettings(ui->plainTextEdit->font(), pixelsFromSpaces(ui->tabSizeSpinBox->value()));
}

void PreferencesDialog::pickUpFont(const QString &name)
{
    QFont font = ui->plainTextEdit->font();
    font.setFamily(name);
    ui->plainTextEdit->setFont(font);

    pickUpTabSize(ui->tabSizeSpinBox->value());
}

void PreferencesDialog::pickUpFontSize(int val)
{
    QFont font = ui->plainTextEdit->font();
    font.setPixelSize(val);
    ui->plainTextEdit->setFont(font);

    pickUpTabSize(ui->tabSizeSpinBox->value());
}

void PreferencesDialog::pickUpTabSize(int val)
{
    ui->plainTextEdit->setTabStopWidth(pixelsFromSpaces(val));

    plainTextUpdateDemo();
}

void PreferencesDialog::readSettings()
{
    QSettings m_settings;

    m_settings.beginGroup(QLatin1String("SerialPort"));
    {
        QString pn = m_settings.value(QLatin1String("portName"), "").toString();

        qint32 br = m_settings.value(QLatin1String("baudRate"), ui->cmbSpeed->itemText(0)).toInt();
        ui->cmbSpeed->setCurrentIndex(ui->cmbSpeed->findText(QString("%1").arg(br)));
        br = ui->cmbSpeed->currentText().toInt();

        emit openPort(pn, br);
    }
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("LogWidget"));
    {
        const QString &font_str = m_settings.value(QLatin1String("font"), ui->plainTextEdit->font().toString()).toString();

        QFont font;
        if (font.fromString(font_str))
        {
            pickUpFont(font.family());
        }
        else
        {
            qDebug() << "WARNING Can't pick up font setting:" << font_str;
        }
        ui->fontComboBox->setCurrentIndex(ui->fontComboBox->findText(ui->plainTextEdit->font().family()));

        pickUpFontSize(QFontInfo(ui->plainTextEdit->font()).pixelSize());
        ui->fontSizeSpinBox->setValue(QFontInfo(ui->plainTextEdit->font()).pixelSize());

        int tabSize = m_settings.value(QLatin1String("tabSize"), 8).toInt();
        ui->tabSizeSpinBox->setValue(tabSize);
        pickUpTabSize(ui->tabSizeSpinBox->value());

        m_mainWindow->setLogWidgetSettings(ui->plainTextEdit->font(), pixelsFromSpaces(ui->tabSizeSpinBox->value()));
    }
    m_settings.endGroup();
}

void PreferencesDialog::writeSettings()
{
    QSettings m_settings;

    m_settings.beginGroup(QLatin1String("SerialPort"));
    {
        m_settings.setValue(QLatin1String("portName"), m_mainWindow->currentPortName());
        m_settings.setValue(QLatin1String("baudRate"), ui->cmbSpeed->currentText());
    }
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("LogWidget"));
    {
        m_settings.setValue(QLatin1String("font"), m_mainWindow->logWidgetFont().toString());

        m_settings.setValue(QLatin1String("tabSize"), ui->tabSizeSpinBox->value());
    }
    m_settings.endGroup();
}

void PreferencesDialog::plainTextUpdateDemo()
{
    ui->plainTextEdit->clear();

    int tabStopWidthSpaces = ui->tabSizeSpinBox->value();

    QString spaces;
    for (int i = 0; i < tabStopWidthSpaces; ++i) {
        spaces += " ";
    }

    ui->plainTextEdit->appendBytes(QString("\x1B[7m	\x1B[mthis is \x1B[1mtabbed\x1B[m text (\x1B[4m%1 spaces, %2 pixels\x1B[m)\n\r\x1B[7m%3\x1B[mthis is \x1B[1mspace-padded\x1B[m text")
                                    .arg(tabStopWidthSpaces).arg(pixelsFromSpaces(tabStopWidthSpaces)).arg(spaces).toLatin1());
}

int PreferencesDialog::pixelsFromSpaces(int spaceCount)
{
    QString spaces;
    for (int i = 0; i < spaceCount; ++i) {
        spaces += " ";
    }

    QFontMetrics fm(ui->plainTextEdit->font());
    return fm.width(spaces);
}
