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

    // XXX QColorDialog crashes on macOS

    ui->textShadeSlider->setMaximum(255);
    ui->bgShadeSlider->setMaximum(255);

    connect(ui->textShadeSlider, SIGNAL(valueChanged(int)), this, SLOT(pickUpTextShadeValue(int)));
    connect(ui->bgShadeSlider, SIGNAL(valueChanged(int)), this, SLOT(pickUpBgShadeValue(int)));
    connect(ui->fontComboBox, SIGNAL(activated(QString)), this, SLOT(pickUpFont(QString)));
    connect(ui->fontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(pickUpFontSize(int)));

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

    m_mainWindow->openSerialDevice(ui->cmbDevice->currentText(), ui->cmbSpeed->currentText().toUInt());
    m_mainWindow->setLogWidgetSettings(ui->plainTextEdit->font(), ui->plainTextEdit->palette());
}

void PreferencesDialog::pickUpTextShadeValue(int val)
{
    m_textShade = val;

    QColor color(val, val, val);
    QPalette palette = ui->plainTextEdit->palette();
    palette.setColor(QPalette::Text, color);
    ui->plainTextEdit->setPalette(palette);

    ui->plainTextEdit->setPlainText(QString("Text shade: %1\nBackground shade: %2\n").arg(m_textShade).arg(m_bgShade));
}

void PreferencesDialog::pickUpBgShadeValue(int val)
{
    m_bgShade = val;

    QColor color(val, val, val);
    QPalette palette = ui->plainTextEdit->palette();
    palette.setColor(QPalette::Base, color);
    ui->plainTextEdit->setPalette(palette);

    ui->plainTextEdit->setPlainText(QString("Text shade: %1\nBackground shade: %2\n").arg(m_textShade).arg(m_bgShade));
}

void PreferencesDialog::pickUpFont(const QString &name)
{
    QFont font = ui->plainTextEdit->font();
    font.setFamily(name);
    ui->plainTextEdit->setFont(font);
}

void PreferencesDialog::pickUpFontSize(int val)
{
    QFont font = ui->plainTextEdit->font();
    font.setPixelSize(val);
    ui->plainTextEdit->setFont(font);
}

void PreferencesDialog::readSettings()
{
    QSettings m_settings;

    m_settings.beginGroup(QLatin1String("SerialPort"));
    {
        QString port = m_settings.value(QLatin1String("portName"), "").toString();

        qint32 baudRate = m_settings.value(QLatin1String("baudRate"), ui->cmbSpeed->itemText(0)).toInt();
        ui->cmbSpeed->setCurrentIndex(ui->cmbSpeed->findText(QString("%1").arg(baudRate)));

        m_mainWindow->openSerialDevice(port, baudRate);
    }
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("LogWidget"));
    {
        int text_shade = m_settings.value(QLatin1String("textShade"), 255).toInt();
        ui->textShadeSlider->setValue(text_shade);
        pickUpTextShadeValue(ui->textShadeSlider->value());

        int bg_shade = m_settings.value(QLatin1String("bgShade"), 0).toInt();
        ui->bgShadeSlider->setValue(bg_shade);
        pickUpBgShadeValue(ui->bgShadeSlider->value());

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

        m_mainWindow->setLogWidgetSettings(ui->plainTextEdit->font(), ui->plainTextEdit->palette());
    }
    m_settings.endGroup();
}

void PreferencesDialog::writeSettings()
{
    QSettings m_settings;

    m_settings.beginGroup(QLatin1String("SerialPort"));
    {
        m_settings.setValue(QLatin1String("portName"), m_mainWindow->currentPortName());
        m_settings.setValue(QLatin1String("baudRate"), m_mainWindow->currentBaudRate());
    }
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("LogWidget"));
    {
        m_settings.setValue(QLatin1String("textShade"), m_textShade);
        m_settings.setValue(QLatin1String("bgShade"), m_bgShade);

        m_settings.setValue(QLatin1String("font"), m_mainWindow->logWidgetFont().toString());
    }
    m_settings.endGroup();
}
