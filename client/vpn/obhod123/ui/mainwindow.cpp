#include <QMetaEnum>
#include <QMovie>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScroller>
#include <QScrollBar>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QGridLayout>


#include "flags.h"
#include "mainwindow.h"
#include "backend/vpnutils.h"
#include "backend/vpndefine.h"


#ifdef Q_OS_WIN
#include "ui_mainwindow.h"
#endif

#ifdef Q_OS_MAC
#include "ui_mainwindow_mac.h"
#include "publib/macos_functions.h"
#endif

#include "backend/sshclient.h"

QString FormatDataNumber(quint64 size)
{
    double size_d = size;

    if(size_d < 1000)
        return QString::number(size_d, 'f', 0);
    size_d /= 1000;
    if(size_d < 1000)
        return QString::number(size_d, 'f', 0) + "K";
    size_d /= 1000;
    if(size_d < 1000)
        return QString::number(size_d, 'f', 2) + "M";
    size_d /= 1000;
    if(size_d < 1000)
        return QString::number(size_d, 'f', 2) + "G";
    size_d /= 1000;
    return QString::number(size_d, 'f', 2) + "T";
}


MainWindow::MainWindow(bool useForceUseBrightIcons, QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),
    m_receiveSpeedNext(0),
    m_sendSpeedNext(0),
    m_receiveSpeedPrev(0),
    m_sendSpeedPrev(0),
    needAutoLogin(false),
    needExit(false),
    forceUseBrightIcons(useForceUseBrightIcons)
{
    ui->setupUi(this);
    qsrand(QTime::currentTime().msecsSinceStartOfDay());

    //ui->label_status_obhod_valid->hide();
    //ui->pushButton_pay->hide();

//    NetworkController::setDnsServersOnHardwareAdapters("8.8.8.8", "8.8.8.4");
//    return;


    initBlockedSites();
    initCustomSites();

    loadSetting();
    setSettingsConnections();

    checkIfFirstRun();
    License::checkLicenseOnStartup();

    ui->widget_tittlebar->installEventFilter(this);


#ifdef Q_OS_WIN
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    setupGraphicsEffects();

   // ui->widget_tittlebar->installEventFilter(this);
#endif
    //canMove = false;
    //ui->label_detected_flag->setScaledContents(true);

    ui->stackedWidget_main->setCurrentWidget(ui->page_init);

    ui->stackedWidget_main->setSpeed(300);
    ui->stackedWidget_main->setAnimation(QEasingCurve::Linear);
    setupTray();

    setVpnConnections();

    qRegisterMetaType<VPNClient::STATE>("VPNClient::STATE");

    connect(&durationTimer, &QTimer::timeout, [&](){
#if QT_VERSION > QT_VERSION_CHECK(5, 7, 0)
//        ui->label_duration->setText(secondsToString(QDateTime::currentSecsSinceEpoch() - connectedDateTime));
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
        ui->label_duration->setText(secondsToString(QDateTime::currentDateTime().toTime_t() - connectedDateTime));
#endif
    });
    //durationTimer.start(1000);  // will update label with connect duration


//    QMovie *conn_animation = new QMovie(":/images/connecting_animation.gif");
//    conn_animation->setScaledSize(QSize(212, 212));
//    ui->label_conn_animation->setMovie(conn_animation);
//    conn_animation->start();

//    QMovie *darknet_animation = new QMovie(":/images/anim.gif");
//    darknet_animation->setScaledSize(QSize(142, 122));
//    darknet_animation->jumpToFrame(20);
//    ui->label_login_animation->setMovie(darknet_animation);
//    darknet_animation->start();
//    ui->label_init_animation->setMovie(darknet_animation);
//    // wait while showAnimated() finished and unpin label_login_animation to current position


    {
        // traffic counters
        speedCounterRequestTimer.setInterval(1000);
        connect(&speedCounterRequestTimer, &QTimer::timeout, this, &MainWindow::onSpeedCounterRequest);
        speedCounterRequestTimer.start();

        speedCounterRefreshTimer.setInterval(300);
        connect(&speedCounterRefreshTimer, &QTimer::timeout, this, &MainWindow::onSpeedCounterRefresh);
        speedCounterRefreshTimer.start();
    }


    QTimer::singleShot(3000, [&](){
        setupInitPage();
        //on_pushButton_pay_clicked();
    });


#ifdef Q_OS_MAC
    setFixedSize(width(), height());
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
#endif


    //resize(width(), QApplication::desktop()->availableGeometry().height());
    ui->widget_main->resize(ui->widget_main->width(), QApplication::desktop()->availableGeometry().height());
    ui->stackedWidget_main->resize(ui->stackedWidget_main->width(), QApplication::desktop()->availableGeometry().height());
    showObhod123();

    onConnect();
}

MainWindow::~MainWindow()
{
    needExit = true;

    onDisconnect();

    hide();

    QEventLoop loop;
    QTimer::singleShot(2000, &loop, SLOT(quit()));
    loop.exec();

    m_tray.hide();

    disconnect(this, 0, 0, 0);
    if (obhod123.getVpnClient()) disconnect(obhod123.getVpnClient(), 0, 0, 0);

    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != ui->widget_tittlebar) return false;
    QWidget *w = qobject_cast<QWidget *>(obj);

     if (event->type() == QEvent::MouseMove) {
         QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
         QPoint pos = mouseEvent->pos();
         QPoint globalPos = mouseEvent->globalPos();

         if (canMove) {
             int newX = x() - (initialMovePoint.x() - mapFromGlobal(globalPos).x());
             int newY = y() - (initialMovePoint.y() - mapFromGlobal(globalPos).y() + 10);

             move(newX, newY);
         }
         return true;
     }
     else if (event->type() == QEvent::MouseButtonPress) {
         QMouseEvent *pressEvent = static_cast<QMouseEvent *>(event);
         canMove = true;
         //initialMovePoint = pressEvent->globalPos();
         initialMovePoint = pressEvent->pos();

         return false;
     }
     else if (event->type() == QEvent::MouseButtonRelease) {
         QMouseEvent *pressEvent = static_cast<QMouseEvent *>(event);
         canMove = false;

         return false;
     }

     event->ignore();
     return false;
}

void MainWindow::on_pushButton_close_clicked()
{
    needExit = true;
    qApp->quit();
}

//bool MainWindow::eventFilter(QObject *obj, QEvent *event)
//{
//    if (obj != ui->widget_tittlebar) return false;
//    //QWidget *w = qobject_cast<QWidget *>(obj);

//    if (event->type() == QEvent::MouseMove) {
//        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
//        //QPoint pos = mouseEvent->pos();
//        QPoint globalPos = mouseEvent->globalPos();

//        if (canMove) {
//            int newX = x() - (initialMovePoint.x() - mapFromGlobal(globalPos).x());
//            int newY = y() - (initialMovePoint.y() - mapFromGlobal(globalPos).y() + 10);

//            move(newX, newY);
//        }
//        return true;
//    }
//    else if (event->type() == QEvent::MouseButtonPress) {
//        QMouseEvent *pressEvent = static_cast<QMouseEvent *>(event);
//        canMove = true;
//        //initialMovePoint = pressEvent->globalPos();
//        initialMovePoint = pressEvent->pos();

//        return false;
//    }
//    else if (event->type() == QEvent::MouseButtonRelease) {
//        //QMouseEvent *pressEvent = static_cast<QMouseEvent *>(event);
//        canMove = false;
//        return false;
//    }

//    event->ignore();
//    return false;
//}

QString MainWindow::secondsToString(qint64 seconds)
{
    const qint64 DAY = 86400;
    qint64 days = seconds / DAY;
    QTime t = QTime(0,0).addSecs(seconds % DAY);

    QString timeSpan;
    if (days > 0) {
        timeSpan = QString("%1d, %2")
                .arg(days).arg(t.toString("HH:mm:ss"));
    }
    else {
        timeSpan = t.toString("HH:mm:ss");
    }
    return timeSpan;
}

void MainWindow::on_pushButton_login_clicked()
{
//    if (ui->lineEdit_username->text().isEmpty()) return;
//    //if (ui->lineEdit_password->text().isEmpty()) return;

//    const QJsonObject &authData = API_HTTP::Instance().authData();
//    if (authData.value("error").toString() == "0") return;  // already logged in


//    Settings::setUserId(ui->lineEdit_username->text());

//    ui->label_login_animation->show();
//    ui->pushButton_login->setEnabled(false);



//    // request auth from servers
//    int retry = 0;
//    while (!needExit) {
//        if (retry > 2) break;
//        retry++;

//        QString error = API_HTTP::Instance().requestAuth(ui->lineEdit_username->text());

//        // break on any server answer
//        if (error == "0") {
//            ui->stackedWidget_main->slideInWidget(ui->page_connection, SlidingStackedWidget::AUTOMATIC);
//            needAutoLogin = true;

//            if (Settings::hasConnectWhenStarted()) {
//                QTimer::singleShot(1000, [&](){
//                    onConnect();
//                });
//            }

//            break;
//        }

//        if (error == "1") {
//            ui->label_errorMessage->show();
//            ui->label_errorMessage->raise();
//            ui->label_errorMessage->setText(tr("Login or Password incorrect"));
//            //ui->label_login_animation->hide();

//            break;
//        }

//        else {
//            qDebug() << "MainWindow::on_pushButton_login_clicked :: Failed to auth, retry" << retry;
//            ui->label_errorMessage->show();
//            ui->label_errorMessage->raise();
//            ui->label_errorMessage->setText(QString("Login retry #%1").arg(retry));
//            QEventLoop waitingLoop;
//            QTimer::singleShot(10000, &waitingLoop, SLOT(quit()));
//            waitingLoop.exec();
//        }

//    }

//    ui->pushButton_login->setEnabled(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
}

void MainWindow::setVpnConnections()
{
//    connect(&API_HTTP::Instance(), &API_HTTP::serverListChanged, this, &MainWindow::onServerListChanged);

    connect(&obhod123, &Obhod123::vpnClientStateChanged, this, &MainWindow::onVpnStateChanged);
    connect(&obhod123, &Obhod123::vpnClientMessage, this, &MainWindow::onVpnMessage);

    connect(&m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

void MainWindow::setupObhodPage()
{
    //ui->pushButton_support->hide();

    API_HTTP::Instance().clearAuthData();
//    ui->label_errorMessage->hide();

    ui->stackedWidget_main->slideInWidget(ui->page_obhod, SlidingStackedWidget::AUTOMATIC);

    Settings::setReconnectOnDrop(true);
    Settings::setLaunchOnStartup(true);
    Settings::setConnectWhenStarted(true);
    Settings::setAutoServer(true);
    qApp->processEvents();

#ifdef Q_OS_WIN
        winhelpLaunchStartup("Obhod123", true, NULL);
#endif

    QTimer::singleShot(100, [&](){
        onConnect();
    });
}

void MainWindow::setupInitPage()
{
    ui->stackedWidget_main->slideInWidget(ui->page_obhod, SlidingStackedWidget::AUTOMATIC);
}

void MainWindow::mayBeAutoLogin()
{
    if (Settings::launchOnStartup() && !Settings::userId().isEmpty()) {

        // check wait for qapp started
        QTimer *t0 = new QTimer;
        t0->setSingleShot(true);
        connect(t0, &QTimer::timeout, [&](){
            QTimer *t1 = new QTimer;
            t1->setSingleShot(true);
            connect(t1, &QTimer::timeout, [&](){
                // try to connect later in case if no internet on startup
                // app will wait singal "onlineStateChanged"==online and then try to connect
                needAutoLogin = true;
                connect(&API_HTTP::Instance().confManager, &QNetworkConfigurationManager::onlineStateChanged, [&](bool isOnline){
                    if (isOnline && needAutoLogin) {
                        QTimer::singleShot(3000, [&](){
                            on_pushButton_login_clicked();
                        });
                    }
                });
                on_pushButton_login_clicked();
            });
            t1->start(300);
        });
        t0->start(0);
    }
}

void MainWindow::on_pushButton_cancel_clicked()
{
    //ui->stackedWidget_main->slideInWidget(ui->page_connection, SlidingStackedWidget::AUTOMATIC);
}

void MainWindow::on_checkBox_lauchOnStartup_clicked()
{

    //Settings::setLaunchOnStartup(ui->checkBox_lauchOnStartup->isChecked());
}

void MainWindow::on_pushButton_connect_clicked()
{
    onConnect();
}

void MainWindow::on_pushButton_cancelConnect_clicked()
{
    onCancelConnect();
}

void MainWindow::on_pushButton_disconnect_clicked()
{
    onDisconnect();
}

void MainWindow::onConnect()
{
    // ruturn if connected
    if (obhod123.vpnStatus() == VPNClient::CONNECTED) return;

    lastUserAction = Connect;

//    if (q_settings.value("protocol").toString() == "ovpn")
//        ui->widget_conn_speed->show();
//    else
//        ui->widget_conn_speed->hide();


//    ui->stackedWidget_center->setCurrentWidget(ui->page_center_connecting);
//    QString currentServerId;
//    if (ui->checkBox_autoServer->isChecked())
//        currentServerId = "";
//    else
//        currentServerId = Settings::currentServerId();


    QJsonObject connectionParams;
    connectionParams.insert("server", Settings::currentServerIp());
    connectionParams.insert("protocol", Settings::protocol());


    if (Settings::appMode() == "obhod") {
        while (!needExit) {
            bool ok = obhod123.onConnectAuto(connectionParams);
            if (ok) break;

            QEventLoop loop;
            QTimer::singleShot(10000, &loop, SLOT(quit()));
            loop.exec();
        }

        qDebug() << "MainWindow::onConnect :::: Obhod mode : onConnectAuto success";
    }
    else {
        bool ok = obhod123.onConnectAuto(connectionParams);
        if (!ok) {
            //ui->stackedWidget_center->setCurrentWidget(ui->page_center_disconnected);
            QMessageBox::warning(0, "Obhod123", "Couldn't connect to any server.\n");
        }
        qDebug() << "MainWindow::onConnect :::: Pro mode : onConnectAuto success";


    }
}

void MainWindow::onDisconnect()
{
    lastUserAction = Disconnect;

//    ui->pushButton_disconnect->setEnabled(false);
//    ui->pushButton_cancelConnect->setEnabled(false);

    obhod123.onClickDisconnect();
}

void MainWindow::onCancelConnect()
{
    lastUserAction = CancelConnect;

//    ui->pushButton_cancelConnect->setEnabled(false);
//    ui->pushButton_disconnect->setEnabled(false);

    obhod123.onCancel();
}


void MainWindow::onVpnStateChanged(VPNClient::STATE state)
{
//    QString currentServerId;
//    if (ui->checkBox_autoServer->isChecked())
//        currentServerId = "";
//    else
//        currentServerId = Settings::currentServerId();

    QString currentServerIp = Settings::currentServerIp();
    //QMetaEnum metaEnum = QMetaEnum::fromType<VPNClient::STATE>();
    //QString stateName = metaEnum.valueToKey(state);




    qDebug().noquote() << "MainWindow::onVpnStateChanged: new STATE: " << state;

    // Set as default - we set this action later
    m_trayActionConnect->setEnabled(false);

    if (state == VPNClient::CONNECTING) {
//        ui->stackedWidget_center->setCurrentWidget(ui->page_center_connecting);
//        ui->pushButton_select_server->setEnabled(false);
//        ui->label_status->setText(tr("Connecting..."));
        ui->label_status_2->setText(tr("Connecting..."));
    }

    if (state == VPNClient::DISCONNECTING) {
//        ui->stackedWidget_center->setCurrentWidget(ui->page_center_connecting);
//        ui->label_status->setText(tr("Disconnecting..."));
        ui->label_status_2->setText(tr("Disconnecting..."));
    }
    if (state == VPNClient::DISCONNECTED) {
        durationTimer.stop();
//        ui->label_duration->setText("00:00:00");
//        ui->stackedWidget_center->setCurrentWidget(ui->page_center_disconnected);
//        ui->label_status->setText(tr("Disconnected"));
        ui->label_status_2->setText(tr("Connecting..."));

        setTrayIcon("default.png");


        m_trayActionConnect->setEnabled(true);
        m_trayActionConnect->setText(tr("Connect"));
    }
    if (state == VPNClient::CONNECTED) {
        durationTimer.start(1000);
        connectedDateTime = QDateTime::currentDateTime().toTime_t();
//        ui->stackedWidget_center->setCurrentWidget(ui->page_center_connected);
//        ui->label_status->setText(tr("Connected"));
        ui->label_status_2->setText(tr("Connected"));
//        ui->pushButton_cancelConnect->setEnabled(true);
//        ui->pushButton_disconnect->setEnabled(true);


        setTrayIcon("active.png");


        m_trayActionConnect->setEnabled(true);
        m_trayActionConnect->setText(tr("Disconnect"));

        {
            // hide windows in case if mouse outside of the window
            QPointer<QTimer> t = new QTimer;
            bool *needHideAfterConnect = new bool(true);
            connect(t.data(), &QTimer::timeout, [&, needHideAfterConnect](){
                if (rect().contains(mapFromGlobal(QCursor::pos()))) *needHideAfterConnect = false;
            });
            t->start(50);

            QTimer::singleShot(3000, [&, t, needHideAfterConnect](){
                if (isVisible() && *needHideAfterConnect) hideAnimated();
                if (t) t->stop();
                if (t) t->deleteLater();
                if (needHideAfterConnect) delete needHideAfterConnect;
            });
        }

    }
    if (state == VPNClient::VPNERROR) {      
//        ui->stackedWidget_center->setCurrentWidget(ui->page_center_disconnected);
//        if (!ui->checkBox_autoServer->isChecked()) ui->pushButton_select_server->setEnabled(true);
        //ui->stackedWidget_bottom->setCurrentWidget(ui->page_bottom_disconnected);
        //ui->label_status->setText(tr("VPN Error"));
        ui->label_status_2->setText(tr("VPN Error"));

        setTrayIcon("error.png");

        //ui->label_detected_location->setText(originalIp + ", " + originalRegion);

        m_trayActionConnect->setEnabled(true);
        m_trayActionConnect->setText(tr("Connect"));

        if (Settings::reconnectOnDrop()
                && lastUserAction == Connect
                && obhod123.autoConnectInProgress == false) {

            QTimer::singleShot(5000, [&](){
                onConnect();
            });
        }
    }


//    /// Display tray messages depends of server
//    if (currentServerIp.isEmpty()) {
//        // if auto server
//        //if (state == VPNClient::CONNECTED) m_tray.showMessage("Obhod123", "VPN Connected");
//        //if (state == VPNClient::DISCONNECTED) m_tray.showMessage("Obhod123", "VPN Disconnected");
//    }
//    else {
//        //if (state == VPNClient::CONNECTED) m_tray.showMessage("Obhod123", "VPN Connected");
//        //if (state == VPNClient::DISCONNECTED) m_tray.showMessage("Obhod123", "VPN Disconnected");
//        //if (state == VPNClient::CONNECTING) m_tray.showMessage("Obhod123", "Connecting to VPN...");
//    }

}

void MainWindow::onVpnMessage(QString msg)
{
    /// Used for displaying forwarded port
    if(msg.contains("network/local/netmask")) {
        //qDebug() << "MainWindow::onVpnMessage :::: " << msg;

        QString ip = Obhod123Utils::getStringBetween(msg, "network/local/netmask", "\r\n");
        QStringList ips = ip.split("/");
        if(ips.size() < 2)
            return;
        ip = ips.at(1);
        qDebug() << "Local IP: " << ip;

        if (ip.split(".").size() < 4) return;

        // Get last octet of ip address
        // Prepend '0' to make 'ddd' format
        QString _4th_octet = ip.split(".").at(3);
        _4th_octet.prepend("000");
        _4th_octet = _4th_octet.mid(_4th_octet.size() - 3);

        // Port is "51" + _4th_octet
        QString port_forwarded = "51" + _4th_octet;
        //ui->label_port_forward->setText("Port forwarded: " + port_forwarded);
    }
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    //qDebug().noquote() << reason;
    if(reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger)
        showObhod123();
}

void MainWindow::showObhod123()
{
    showAnimated();
    return;

//    resize(width(), QApplication::desktop()->availableGeometry().height());
//    move(QApplication::desktop()->availableGeometry().width() - width(), 0);

//    showNormal();
//    activateWindow();
//    raise();

//    move((qApp->desktop()->width() - width()) / 2,
//         (qApp->desktop()->height() - height()) / 2);
//    showNormal();
//    activateWindow();
    //    raise();
}

void MainWindow::showAnimated()
{
    show();
    move(0,0);
//    resize(0, QApplication::desktop()->availableGeometry().height());
//    move(QApplication::desktop()->availableGeometry().width(), 0);

//    showNormal();
//    activateWindow();
//    raise();

////    QPropertyAnimation *move1 = new QPropertyAnimation(this, "pos");
////    move1->setStartValue(pos());
////    move1->setEndValue(pos() - QPoint(width(),0));
////    move1->setDuration(200);

////    move1->start();



//    QPropertyAnimation *move2 = new QPropertyAnimation(this, "geometry");
//    move2->setStartValue(QRect(QApplication::desktop()->availableGeometry().width(),0,390,height()));
//    move2->setEndValue(QRect(QApplication::desktop()->availableGeometry().width() - 390,0,390,height()));
//    move2->setDuration(200);
//    move2->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::hideAnimated()
{
    hide();
//    QPropertyAnimation *move2 = new QPropertyAnimation(this, "geometry");
//    move2->setStartValue(geometry());
//    move2->setEndValue(QRect(QApplication::desktop()->availableGeometry().width(),0,0,height()));
//    move2->setDuration(200);
//    move2->start(QAbstractAnimation::DeleteWhenStopped);

}

void MainWindow::setupTray()
{
    m_menu = new QMenu(QApplication::desktop());
    m_menu->setStyleSheet(styleSheet());

    m_menu->addAction(QIcon(":/images/tray/application.png"), tr("Show Obhod123"), this, SLOT(showObhod123()));
    m_menu->addSeparator();
    //m_trayActionConnect = m_menu->addAction(tr("Connect"), this, SLOT(onTrayActionConnect()));
    m_trayActionConnect = new QAction();

    //m_recent = m_menu->addMenu("Recent Connection");
    //connect(m_recent, SIGNAL(triggered(QAction*)), SLOT(onMenuRecent(QAction*)));
    m_menu->addSeparator();

//    m_menu->addAction(QIcon(":/images/tray/lightbulb.png"), tr("Support"), [&](){
//        QDesktopServices::openUrl(QUrl("https://"));
//    });
//    m_menu->addAction(QIcon(":/images/tray/link.png"), tr("Visit Website"), [&](){
//        QDesktopServices::openUrl(QUrl("https://obhod123.com"));
//    });
//    m_menu->addAction(QIcon(":/images/tray/user.png"), tr("Client Area"), [&](){
//        QDesktopServices::openUrl(QUrl("http://secure.fuckconnect.com"));
//    });
//    m_menu->addSeparator();
    //    m_menu->addAction(QIcon(":/images/tray/arrow_refresh.png"), tr("Update Now"), this, SLOT(onUpdate()));
    //    m_menu->addSeparator();

    m_menu->addAction(QIcon(":/images/tray/cancel.png"), tr("Quit Obhod123"), this, [&](){
        QMessageBox msgBox(QMessageBox::Question, tr("Exit"), tr("Do you really want to exit Obhod123?"),
                           QMessageBox::Yes | QMessageBox::No, Q_NULLPTR, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.raise();
        if (msgBox.exec() == QMessageBox::Yes) {
            qApp->quit();
        }


    });

    m_tray.setContextMenu(m_menu);
    setTrayIcon("default.png");

    m_tray.show();
}

void MainWindow::setupGraphicsEffects()
{
    //  Main Window Shadow
    CustomShadowEffect *shadow = new CustomShadowEffect;
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(50,50,50));
    shadow->setDistance(0);
    ui->widget_main->setGraphicsEffect(shadow);
}

void MainWindow::initBlockedSites()
{
    QStringList list;
    list << "https://telegram.org";
    list << "http://rutracker.org";
    list << "https://nnm-club.me";
    list << "https://linkedin.com";
    list << "http://gidonline.in";
    list << "http://torr.ws";
    list << "http://onlinego.club";

    QStringListModel *model = new QStringListModel(list);
    ui->listView_sites->setModel(model);

    connect(ui->listView_sites, &QListView::doubleClicked, [&](const QModelIndex &index){
        QDesktopServices::openUrl(index.data().toString());
    });
}

void MainWindow::initCustomSites()
{
    customSitesModel = new QStringListModel(Settings::customSites());
    ui->listView_sites_custom->setModel(customSitesModel);

    connect(ui->listView_sites_custom, &QListView::doubleClicked, [&](const QModelIndex &index){
        QDesktopServices::openUrl("https://" + index.data().toString());
    });
}

void MainWindow::onTrayActionConnect()
{
    if(m_trayActionConnect->text() == tr("Connect")) {
        onConnect();
    } else if(m_trayActionConnect->text() == tr("Disconnect")) {
        onDisconnect();
    }
}

///
/// \brief MainWindow::onSpeedCounterRequest
/// Request traffic counters.
/// These counters updated every 1 second in OpenVPN
///
void MainWindow::onSpeedCounterRequest()
{
    static quint64 lr, ls;

    m_receiveSpeedPrev = m_receiveSpeedNext;
    m_sendSpeedPrev = m_sendSpeedNext;

    if (!obhod123.getVpnClient()) return;
    if(obhod123.getVpnClient()->state() != VPNClient::CONNECTED) {
        ls = lr = 0;
        return;
    }

    quint64 r = 0, s = 0, rr = 0, rs = 0;
    obhod123.getVpnClient()->getByteCount(r, s);
    rr = r - lr; rs = s - ls;

    if (rr || rs) {
        lr = r;
        ls = s;
    }

    m_sendSpeedNext = rs;
    m_receiveSpeedNext = rr;
}

///
/// \brief MainWindow::onSpeedCounterRefresh
/// Calculate interpolated traffic speed and display it
///
void MainWindow::onSpeedCounterRefresh()
{
    int currentInterval = QTime::currentTime().msec();


    qint64 currentSendSpeed = m_sendSpeedPrev + currentInterval*(m_sendSpeedNext - m_sendSpeedPrev) / 1000.0;
    qint64 currentReceiveSpeed = m_receiveSpeedPrev + currentInterval*(m_receiveSpeedNext - m_receiveSpeedPrev) / 1000.0;

//    ui->label_speed_download->setText(QString("%1bps").arg(FormatDataNumber(currentReceiveSpeed * 8)));
//    ui->label_speed_upload->setText(QString("%1bps").arg(FormatDataNumber(currentSendSpeed * 8)));

    ui->label_speed_download_2->setText(QString("%1bps").arg(FormatDataNumber(currentReceiveSpeed * 8)));
    ui->label_speed_upload_2->setText(QString("%1bps").arg(FormatDataNumber(currentSendSpeed * 8)));
}


void MainWindow::setTrayIcon(const QString& iconName)
{
    QString useIconName = iconName;
    QString resourcesPath = ":/images/tray/%1";

#ifdef Q_OS_MAC
    // Get theme from current user (note, this app can be launched as root application and in this case this theme can be different from theme of real current user )
    bool darkTaskBar = MacOSFunctions::instance().isMenuBarUseDarkTheme();
    darkTaskBar = forceUseBrightIcons ? true : darkTaskBar;
    resourcesPath = ":/images_mac/tray_icon/%1";
    useIconName = useIconName.replace(".png", darkTaskBar ? "@2x.png" : " dark@2x.png");
#endif

    m_tray.setIcon(QIcon(QPixmap(QString(resourcesPath).arg(useIconName)).scaled(128,128)));
}

void MainWindow::on_pushButton_hide_clicked()
{
    hideAnimated();
}

void MainWindow::on_pushButton_logout_clicked()
{
    onDisconnect();
}

void MainWindow::on_lineEdit_username_returnPressed()
{
    on_pushButton_login_clicked();
}

void MainWindow::on_pushButton_blocked_list_clicked()
{
    ui->stackedWidget_main->slideInWidget(ui->page_sites);
}

void MainWindow::on_pushButton_back_from_sites_clicked()
{
    ui->stackedWidget_main->slideInWidget(ui->page_obhod);
}

void MainWindow::on_pushButton_sites_add_custom_clicked()
{
    Router::Instance().flushDns();

    QString newSite = ui->lineEdit_sites_add_custom->text();

    if (newSite.isEmpty()) return;
    if (!newSite.contains(".")) return;

    // get domain name if it present
    newSite.replace("https://", "");
    newSite.replace("http://", "");
    newSite.replace("ftp://", "");

    newSite = newSite.split("/", QString::SkipEmptyParts).first();


    QStringList customSites = Settings::customSites();
    if (!customSites.contains(newSite)) {
        customSites.append(newSite);
        Settings::setCustomSites(customSites);

        QString newIp = Obhod123Utils::getIPAddress(newSite);
        QStringList customIps = Settings::customIps();
        if (!newIp.isEmpty() && !customIps.contains(newIp)) {
            customIps.append(newIp);
            Settings::setCustomIps(customIps);

            // add to routes immediatelly
            if (obhod123.vpnStatus() == Obhod123::VPNStatusConnected) {
                Router::Instance().routeAdd(newIp, obhod123.vpnGate());
            }
        }

        initCustomSites();

        ui->lineEdit_sites_add_custom->clear();
    }
    else {
        qDebug() << "customSites already contains" << newSite;
    }
    Router::Instance().flushDns();
}

void MainWindow::on_pushButton_sites_delete_custom_clicked()
{
    Router::Instance().flushDns();

    QModelIndex index = ui->listView_sites_custom->currentIndex();
    QString siteToDelete = index.data(Qt::DisplayRole).toString();

    if (siteToDelete.isEmpty()) {
        return;
    }

    QString ipToDelete = Obhod123Utils::getIPAddress(siteToDelete);

    QStringList customSites = Settings::customSites();
    customSites.removeAll(siteToDelete);
    qDebug() << "Deleted custom site:" << siteToDelete;
    Settings::setCustomSites(customSites);

    QStringList customIps = Settings::customIps();
    customIps.removeAll(ipToDelete);
    qDebug() << "Deleted custom ip:" << ipToDelete;
    Settings::setCustomIps(customIps);


    initCustomSites();

    Router::Instance().routeDelete(Obhod123Utils::getIPAddress(ipToDelete));
    Router::Instance().flushDns();
}

void MainWindow::on_pushButton_back_from_after_update_clicked()
{
    ui->stackedWidget_main->slideInWidget(ui->page_obhod);
}

void MainWindow::on_pushButton_new_server_clicked()
{
    ui->stackedWidget_main->slideInWidget(ui->page_new_server, SlidingStackedWidget::AUTOMATIC);

    //    const char *hostname = "185.242.84.131";
//    const char *command = "uptime";
//    const char *username    = "root";
//    const char *password    = "4B692iy3RFZu";
//    //const char *password    = "aPgdo45dWUd4";

//    std::vector<const char *> commands;
//    commands.push_back(command);

//    execCommand(hostname, commands, username, password);
}
