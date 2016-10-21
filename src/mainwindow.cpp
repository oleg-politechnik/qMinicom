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

    qRegisterMetaType<AsyncPort::Status>("AsyncSerialPort::Status");

    AsyncPort *port = new AsyncPort();
    port->moveToThread(&m_asyncPortThread);
    connect(&m_asyncPortThread, SIGNAL(started()), port, SLOT(initialize()));
    connect(this, SIGNAL(openSerialPort(QString,qint32)), port, SLOT(openSerialPort(QString,qint32)));
    connect(this, SIGNAL(openLocalShell()), port, SLOT(openLocalShell()));
    connect(this, SIGNAL(closePort()), port, SLOT(closePort()));
    connect(port, SIGNAL(statusChanged(AsyncPort::Status,QString,qint32)), this, SLOT(updatePortStatus(AsyncPort::Status,QString,qint32)));
    connect(port, SIGNAL(dataReceived(QByteArray,bool)), ui->logWidget, SLOT(appendBytes(QByteArray,bool)));
    connect(ui->logWidget, SIGNAL(sendBytes(QByteArray)), port, SLOT(sendData(QByteArray)));

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
    ui->sideMarkView->setVisible(false);

    connect(ui->actionClear, SIGNAL(triggered(bool)), ui->logWidget, SLOT(clear()));
    connect(ui->actionClearToLine, SIGNAL(triggered(bool)), ui->logWidget, SLOT(clearToCurrentContextMenuLine()));

    ui->actionFind->setShortcut(QKeySequence(QKeySequence::Find));
    connect(ui->actionFind, SIGNAL(triggered(bool)), this, SLOT(showFindWidget()));

    ui->statusBar->addPermanentWidget(ui->labelStatus, 1);

    //

    readSettings();

    //

    m_asyncPortThread.start(QThread::HighestPriority);

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

    m_asyncPortThread.quit();
    m_asyncPortThread.wait();
}

const QFont &MainWindow::logWidgetFont()
{
    return ui->logWidget->font();
}

void MainWindow::setLogWidgetSettings(const QFont &font, int tabStopWidthPixels)
{
    ui->logWidget->setFont(font);
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

void MainWindow::updatePortStatus(AsyncPort::Status st, const QString &pn, qint32 br)
{
    QString msg;

    if (!pn.isEmpty())
    {
        msg += tr("%1 ").arg(pn);

        if (st == AsyncPort::Online && br > 0) // br is <= 0 for the local shell
        {
            msg += tr("| %1").arg(br);
        }
        else
        {
            msg += "| ";
            msg += AsyncPort::convertStatusToQString(st);
        }
    }
    else
    {
        msg += AsyncPort::convertStatusToQString(st);
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
    ui->sideMarkView->setVisible(visible);

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

void MainWindow::showFindWidget()
{
    setFindWidgetVisible(true);
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
