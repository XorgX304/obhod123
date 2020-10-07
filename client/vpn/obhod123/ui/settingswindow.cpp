#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "backend/api_http.h"

void MainWindow::checkIfFirstRun()
{
    if (Settings::notFirtstRun()) return;

    if (! Settings::hasInstallDateTime()) {
        Settings::setInstallDateTime(QDateTime::currentDateTime());
    }

    Settings::setAutoServer(true);
    Settings::setProtocol("udp");
    Settings::setConnectWhenStarted(true);
    Settings::setLaunchOnStartup(true);
    Settings::setReconnectOnDrop(true);


    on_checkBox_lauchOnStartup_clicked();
//#ifdef Q_OS_WIN
//        winhelpLaunchStartup("Obhod123", ui->checkBox_LaunchAtStart->isChecked(), NULL);
//#endif
}

void MainWindow::loadSetting()
{
    ui->label_version->setText(QString("v.1.0, %1").arg(__DATE__));

//    // General
//    ui->checkBox_LaunchAtStart->setChecked(Settings::launchOnStartup());
//    ui->checkBox_ConnectWhenStarted->setChecked(Settings::connectWhenStarted());
//    ui->checkBox_ReconnectIfDrop->setChecked(Settings::reconnectOnDrop());
//    ui->checkBox_KillSwitch->setChecked(Settings::networkLock());
}


void MainWindow::setSettingsConnections()
{
//    // General
//    connect(ui->checkBox_LaunchAtStart, &QCheckBox::toggled, [&](){
//        Settings::setLaunchOnStartup(ui->checkBox_LaunchAtStart->isChecked());

//#ifdef Q_OS_WIN
//        winhelpLaunchStartup("Obhod123", ui->checkBox_LaunchAtStart->isChecked(), NULL);
//#endif
//    });

//    connect(ui->checkBox_ConnectWhenStarted, &QCheckBox::toggled, [&](){
//        Settings::setConnectWhenStarted(ui->checkBox_ConnectWhenStarted->isChecked());
//    });

//    connect(ui->checkBox_ReconnectIfDrop, &QCheckBox::toggled, [&](){
//        Settings::setReconnectOnDrop(ui->checkBox_ReconnectIfDrop->isChecked());
//    });

//    connect(ui->checkBox_KillSwitch, &QCheckBox::toggled, [&](){
//        Settings::setNetworkLock(ui->checkBox_KillSwitch->isChecked());
//    });

//    connect(ui->checkBox_accessToNetDevices, &QCheckBox::clicked, [&](){
//        q_settings.setValue("accessToNetDevices", ui->checkBox_accessToNetDevices->isChecked());
    //    });
}


