#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QScrollBar>
#include <QtSerialPort/QtSerialPort>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_port(new QSerialPort(this))
{
    ui->setupUi(this);
    this->setWindowTitle(QCoreApplication::applicationName());

    ui->findWidget->setVisible(false);
    connect(ui->findLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateSearch()));
    connect(ui->csFindBtn, SIGNAL(toggled(bool)), this, SLOT(updateSearch()));
    connect(ui->findNextBtn, SIGNAL(clicked(bool)), ui->logWidget, SLOT(findNext()));
    connect(ui->findLineEdit, SIGNAL(returnPressed()), ui->logWidget, SLOT(findNext()));
    connect(ui->findPrevBtn, SIGNAL(clicked(bool)), ui->logWidget, SLOT(findPrev()));

    ui->logWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->logWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customLogWidgetContextMenuRequested(QPoint)));

    QGraphicsScene *m_scene = new QGraphicsScene(this);
    ui->logWidget->attachSideMarkScene(m_scene);
    ui->sideMarkView->setScene(m_scene);

    connect(m_port, SIGNAL(readyRead()), this, SLOT(readPort()));
    connect(m_port, SIGNAL(aboutToClose()), this, SLOT(portAboutToClose()));
    connect(ui->actionClear, SIGNAL(triggered(bool)), ui->logWidget, SLOT(clear()));

    ui->actionFind->setShortcut(QKeySequence(QKeySequence::Find));
    connect(ui->actionFind, SIGNAL(triggered(bool)), this, SLOT(setFindWidgetVisible(bool)));

    //

    updatePortStatus(m_port->isOpen());

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
        qDebug() << "port" << m_port->portName() << "closed";
    }

    delete ui;
}

const QString MainWindow::currentPortName()
{
    return m_port->portName();
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
            qDebug() << "port" << m_port->portName() << "closed";
        }
        else if (m_port->baudRate() != baudRate)
        {
            m_port->setBaudRate(baudRate);
            qDebug() << "port" << m_port->portName() << "@" << baudRate << "opened";
            updatePortStatus(m_port->isOpen());
        }
    }

    if (!m_port->isOpen())
    {
        m_port->setPortName(portName);

        if (!m_port->open(QSerialPort::ReadWrite))
        {
            qDebug() << "ERROR Can't open" << m_port->portName();
            updatePortStatus(m_port->isOpen());
        }
        else
        {
            m_port->setParity(QSerialPort::NoParity); // todo if
            m_port->setDataBits(QSerialPort::Data8); // todo if
            m_port->setParity(QSerialPort::NoParity); // todo if
            m_port->setStopBits(QSerialPort::OneStop); // todo if
            m_port->setFlowControl(QSerialPort::NoFlowControl); // todo if

            m_port->setBaudRate(baudRate);

            qDebug() << "port" << m_port->portName() << "@" << baudRate << "opened";

            updatePortStatus(m_port->isOpen());
        }
    }
}

void MainWindow::setLogWidgetSettings(const QFont &font, const QPalette &palette, int tabStopWidthPixels)
{
    ui->logWidget->setFont(font);
    ui->logWidget->setPalette(palette);
    ui->logWidget->setTabStopWidth(tabStopWidthPixels);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event); // try processing by the parent class first

    if (!event->isAccepted())
    {
        if (event->type() == QEvent::KeyPress && event->key() == Qt::Key_Escape && ui->findWidget->isVisible())
        {
            setFindWidgetVisible(false);
            ui->actionFind->setChecked(false);
            event->accept();
        }
    }
}

void MainWindow::readPort()
{
    QString text = m_port->readAll();

    text.replace("\r", "");

    ui->logWidget->appendTextNoNewline(text);
}

void MainWindow::updatePortStatus(bool isOpen)
{
    QString msg;

    if (!m_port->portName().isEmpty())
    {
        msg += tr("%1 ").arg(m_port->portName());

        if (isOpen)
        {
            msg += tr("| %1").arg(m_port->baudRate());
        }
        else
        {
            msg += tr("| OFFLINE");
        }
    }
    else
    {
        msg += tr("OFFLINE");
    }

    ui->statusBar->showMessage(msg);
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

void MainWindow::setFindWidgetVisible(bool visible)
{
    QScrollBar *p_scroll_bar = ui->logWidget->verticalScrollBar();
    bool was_at_bottom = (p_scroll_bar->value() == p_scroll_bar->maximum());

    ui->findWidget->setVisible(visible);

    if (visible)
    {
        if (was_at_bottom)
        {
            p_scroll_bar->setValue(p_scroll_bar->maximum());
        }
        ui->findLineEdit->setFocus();
        ui->findLineEdit->selectAll();
    }

    updateSearch();
}

void MainWindow::updateSearch()
{
    if (ui->findWidget->isVisible())
    {
        ui->logWidget->setSearchPhrase(ui->findLineEdit->text(), ui->csFindBtn->isChecked());
    }
    else
    {
        ui->logWidget->setSearchPhrase(QString(), ui->csFindBtn->isChecked());
    }
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
