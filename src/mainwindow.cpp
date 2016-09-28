#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QScrollBar>
#include <QtSerialPort/QtSerialPort>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_port(new QSerialPort(this))
{
    ui->setupUi(this);
    this->setWindowTitle(QCoreApplication::applicationName());

    ui->logWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->logWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customLogWidgetContextMenuRequested(QPoint)));

    connect(m_port, SIGNAL(readyRead()), this, SLOT(readPort()));
    connect(m_port, SIGNAL(aboutToClose()), this, SLOT(portAboutToClose()));
    connect(ui->actionClear, SIGNAL(triggered(bool)), ui->logWidget, SLOT(clear()));

    //

    updatePortStatus(false);

    //

    readSettings();

    //

    this->dlgPrefs = new PreferencesDialog(this);
    connect(ui->actionPrefs, SIGNAL(triggered(bool)), this->dlgPrefs, SLOT(open()));
}

MainWindow::~MainWindow()
{
    writeSettings();

    delete this->dlgPrefs;

    if (m_port->isOpen())
    {
        m_port->close();
    }

    delete ui;
}

const QString MainWindow::currentPortName()
{
    return m_port->portName();
}

qint32 MainWindow::currentBaudRate()
{
    return m_port->baudRate();
}

const QFont &MainWindow::logWidgetFont()
{
    return ui->logWidget->font();
}

void MainWindow::openSerialDevice(const QString &portName, qint32 baudRate)
{
    if (portName.isEmpty())
    {
        return;
    }

    if (m_port->isOpen())
    {
        if (m_port->portName() != portName)
        {
            m_port->close();
        }
    }

    if (m_port->isOpen())
    {
        updatePortStatus(true);
    }
    else
    {
        m_port->setPortName(portName);

        if (!m_port->open(QSerialPort::ReadWrite))
        {
            m_port->setBaudRate(baudRate); // just store

            qDebug() << "ERROR Can't open" << m_port->portName();
            updatePortStatus(false);
        }
        else
        {
            m_port->setParity(QSerialPort::NoParity); // todo if
            m_port->setDataBits(QSerialPort::Data8); // todo if
            m_port->setParity(QSerialPort::NoParity); // todo if
            m_port->setStopBits(QSerialPort::OneStop); // todo if
            m_port->setFlowControl(QSerialPort::NoFlowControl); // todo if

            m_port->setBaudRate(baudRate);

            updatePortStatus(true);
        }
    }
}

void MainWindow::setLogWidgetSettings(const QFont &font, const QPalette &palette)
{
    ui->logWidget->setFont(font);
    ui->logWidget->setPalette(palette);
}

void MainWindow::readPort()
{
    QString text = m_port->readAll();

    text.replace("\r", "");

    ui->logWidget->appendTextNoNewline(text);
}

void MainWindow::updatePortStatus(bool isOpen)
{
    if (!m_port->portName().isEmpty())
    {
        qDebug() << "port" << m_port->portName() << (isOpen ? "opened" : "closed");

        ui->statusBar->showMessage(tr("%1 8N1 | %2 | %3").arg(m_port->baudRate()).arg(isOpen ? "ONLINE" : "OFFLINE").arg(m_port->portName()));
    }
}

void MainWindow::portAboutToClose()
{
    updatePortStatus(false);
}

void MainWindow::customLogWidgetContextMenuRequested(const QPoint &pos)
{
    const QPoint gpos = QWidget::mapToGlobal(pos);

    QMenu *menu = ui->logWidget->createStandardContextMenu();
    menu->addSeparator();
    menu->addAction(tr("Clear"), ui->logWidget, SLOT(clear()));
    menu->exec(gpos);

    delete menu;
}

void MainWindow::readSettings()
{
    QSettings m_settings;

    m_settings.beginGroup(QLatin1String("MainWindow"));
    {
        const QVariant geom = m_settings.value(QLatin1String("geometry"));
        if (geom.isValid()) {
            setGeometry(geom.toRect());
        }
        if (m_settings.value(QLatin1String("maximized"), false).toBool())
            setWindowState(Qt::WindowMaximized);
    }
    m_settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings m_settings;

    m_settings.beginGroup(QLatin1String("MainWindow"));
    const QString maxSettingsKey = QLatin1String("maximized");
    if (windowState() & Qt::WindowMaximized) {
        m_settings.setValue(maxSettingsKey, true);
    } else {
        m_settings.setValue(maxSettingsKey, false);
        m_settings.setValue(QLatin1String("geometry"), geometry());
    }
    m_settings.endGroup();
}
