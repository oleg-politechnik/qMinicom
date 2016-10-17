#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QScrollBar>
#include <QtSerialPort/QtSerialPort>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(QCoreApplication::applicationName());

    qRegisterMetaType<AsyncSerialPort::Status>("AsyncSerialPort::Status");

    AsyncSerialPort *serialPort = new AsyncSerialPort();
    serialPort->moveToThread(&m_asyncSerialPortThread);
    connect(&m_asyncSerialPortThread, SIGNAL(started()), serialPort, SLOT(initialize()));
    connect(this, SIGNAL(openPort(QString,qint32)), serialPort, SLOT(openPort(QString,qint32)));
    connect(this, SIGNAL(closePort()), serialPort, SLOT(closePort()));
    connect(serialPort, SIGNAL(statusChanged(AsyncSerialPort::Status,QString,qint32)),
            this, SLOT(updatePortStatus(AsyncSerialPort::Status,QString,qint32)));
    connect(serialPort, SIGNAL(dataReceived(QByteArray)), ui->logWidget, SLOT(appendBytes(QByteArray)));
    connect(ui->logWidget, SIGNAL(sendBytes(QByteArray)), serialPort, SLOT(sendData(QByteArray)));

    ui->findWidget->setVisible(false);
    connect(ui->findLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateSearch()));
    connect(ui->csFindBtn, SIGNAL(toggled(bool)), this, SLOT(updateSearch()));
    connect(ui->findNextBtn, SIGNAL(clicked(bool)), ui->logWidget, SLOT(findNext()));
    connect(ui->findLineEdit, SIGNAL(returnPressed()), ui->logWidget, SLOT(findNext()));
    connect(ui->findPrevBtn, SIGNAL(clicked(bool)), ui->logWidget, SLOT(findPrev()));

    ui->logWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->logWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customLogWidgetContextMenuRequested(QPoint)));

    QGraphicsScene *m_scene = new QGraphicsScene(this);
    ui->logWidget->setSideMarkScene(m_scene);
    ui->sideMarkView->setScene(m_scene);

    connect(ui->actionClear, SIGNAL(triggered(bool)), ui->logWidget, SLOT(clear()));
    connect(ui->actionClearToLine, SIGNAL(triggered(bool)), ui->logWidget, SLOT(clearToCurrentContextMenuLine()));

    ui->actionFind->setShortcut(QKeySequence(QKeySequence::Find));
    connect(ui->actionFind, SIGNAL(triggered(bool)), this, SLOT(setFindWidgetVisible(bool)));

    ui->statusBar->addPermanentWidget(ui->labelStatus, 1);

    //

    readSettings();

    //

    m_asyncSerialPortThread.start(QThread::HighestPriority);

    //

    m_dlgPrefs = new PreferencesDialog(this);
    connect(ui->actionPrefs, SIGNAL(triggered(bool)), m_dlgPrefs, SLOT(open()));
}

MainWindow::~MainWindow()
{
    writeSettings();

    delete m_dlgPrefs;

    emit closePort();

    delete ui;

    m_asyncSerialPortThread.quit();
    m_asyncSerialPortThread.wait();
}

const QString &MainWindow::currentPortName()
{
    return m_portName;
}

const QFont &MainWindow::logWidgetFont()
{
    return ui->logWidget->font();
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

void MainWindow::updatePortStatus(AsyncSerialPort::Status st, const QString &pn, qint32 br)
{
    QString msg;

    m_portName = pn;

    if (!pn.isEmpty())
    {
        msg += tr("%1 ").arg(pn);

        if (st == AsyncSerialPort::Online)
        {
            msg += tr("| %1").arg(br);
        }
        else
        {
            msg += "| ";
            msg += AsyncSerialPort::convertStatusToQString(st);
        }
    }
    else
    {
        msg += AsyncSerialPort::convertStatusToQString(st);
    }

    ui->labelStatus->setText(msg);
}

void MainWindow::customLogWidgetContextMenuRequested(const QPoint &pos)
{
    const QPoint gpos = QWidget::mapToGlobal(pos);

    QMenu *menu = ui->logWidget->createStandardContextMenu();
    menu->addSeparator();
    menu->addAction(ui->actionClear);
    menu->addAction(ui->actionClearToLine);

    QTextCursor cur = ui->logWidget->cursorForPosition(pos);
    ui->logWidget->setContextMenuTextCursor(cur);

    menu->exec(gpos); // blocking operation

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
