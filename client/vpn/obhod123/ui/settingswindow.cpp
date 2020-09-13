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
#ifdef Q_OS_WIN
        winhelpLaunchStartup("Obhod123", ui->checkBox_LaunchAtStart->isChecked(), NULL);
#endif
}

void MainWindow::activateLicenseRestrictions()
{
    ui->label_status_2->setText(tr("Error\nLicense Expired"));

    ui->label_sites_add_custom->setEnabled(false);
    ui->lineEdit_sites_add_custom->setEnabled(false);
    ui->pushButton_sites_add_custom->setEnabled(false);
    ui->listView_sites_custom->setEnabled(false);

    ui->label_pay_activate_free->show();
    ui->pushButton_pay_activate_free->show();
}

void MainWindow::deActivateLicenseRestrictions()
{
    ui->label_status_2->setText(tr("Starting..."));

    ui->label_sites_add_custom->setEnabled(true);
    ui->lineEdit_sites_add_custom->setEnabled(true);
    ui->pushButton_sites_add_custom->setEnabled(true);
    ui->listView_sites_custom->setEnabled(true);

    ui->label_pay_activate_free->hide();
    ui->pushButton_pay_activate_free->hide();
}

void MainWindow::displayLicenseInfo()
{
    License::LicenseMode currentLicenseMode = License::getCurrentLicenseMode();
    QString currentAppMode = Settings::appMode();

    qDebug() << "MainWindow::displayLicenseInfo licenseKey:" << Settings::userId();
    qDebug() << "MainWindow::displayLicenseInfo currentLicenseMode:" << currentLicenseMode;
    qDebug() << "MainWindow::displayLicenseInfo currentAppMode:" << currentAppMode;
    qDebug() << "MainWindow::displayLicenseInfo licenseValidDateTime():" << Settings::licenseValidDateTime();
    qDebug() << "MainWindow::displayLicenseInfo isTrialPeriodActive():" << License::isTrialPeriodActive();

    ui->label_pay_license_key->setText(Settings::userId());

    QString label_valid_text;
    QString label_valid_value;
    QString label_valid_color;

    QString label_mode_text = tr("License type:");
    QString label_mode_value;


    // Highlight expired labels
    if (License::isLicenseExpired()) {
        label_valid_text = tr("Expired at:");
        label_valid_color = "red";

        activateLicenseRestrictions();
    }
    else {
        label_valid_text = tr("Valid till:");
        label_valid_color = "green";

        deActivateLicenseRestrictions();
    }

    label_valid_value = Settings::licenseValidDateTime().date().toString();

    if (currentLicenseMode == License::LicenseMode::FREE) {
        label_mode_value = tr("Free");
    }
    else if (currentLicenseMode == License::LicenseMode::BASIC) {
        label_mode_value = tr("Basic");
    }
    else if (currentLicenseMode == License::LicenseMode::PRO) {
        label_mode_value = tr("Pro");
    }
    else if (currentLicenseMode == License::LicenseMode::PREMIUM) {
        label_mode_value = tr("Premium");
    }
    else if (currentLicenseMode == License::LicenseMode::TRIAL) {
        if (currentAppMode == "obhod") {
            label_mode_value = tr("Basic (7 days trial)");
        }
        else if (currentAppMode == "pro") {
            label_mode_value = tr("Pro (7 days trial)");
        }
        else if (currentAppMode == "premium") {
            label_mode_value = tr("Premium (7 days trial)");
        }
    }

    // obhod page
    {
        style()->unpolish(ui->label_obhod_license_valid);
        ui->label_obhod_license_valid->setText(label_valid_text + "  " + label_valid_value);
        ui->label_obhod_license_valid->setProperty("valid_color", label_valid_color);
        style()->polish(ui->label_obhod_license_valid);

        ui->label_obhod_license_type->setText(label_mode_text + " " + label_mode_value);
    }

    // pay page
    {
        style()->unpolish(ui->label_pay_license_valid);
        ui->label_pay_license_valid->setText(label_valid_text + "  " + label_valid_value);
        ui->label_pay_license_valid->setProperty("valid_color", label_valid_color);
        style()->polish(ui->label_pay_license_valid);

        ui->label_pay_license_type->setText(label_mode_text + " " + label_mode_value);
    }



}


void MainWindow::initTarifSettings()
{
    // License types :
    // 1. "free"
    // 2. "trial"
    // 3. "basic"
    // 4. "pro"
    // 5. "premium"


    // App modes :
    // 1. "obhod"
    // 2. "pro"
    // 3. "premium"

    // check license and setup corresponding page
    License::LicenseMode currentLicenseMode = License::getCurrentLicenseMode();
    QString currentAppMode = Settings::appMode();

    if (currentLicenseMode == License::LicenseMode::FREE
            || currentLicenseMode == License::LicenseMode::BASIC
            || (currentLicenseMode == License::LicenseMode::TRIAL && currentAppMode == "obhod")) {
        API_HTTP::Instance().requestBlockedIps();

        // setup obhod page
        setupObhodPage();
    }
    else if (currentLicenseMode == License::LicenseMode::PRO || (currentLicenseMode == License::LicenseMode::TRIAL && currentAppMode == "pro")) {
        // setup pro connection page
    }
    else if (currentLicenseMode == License::LicenseMode::PREMIUM || (currentLicenseMode == License::LicenseMode::TRIAL && currentAppMode == "premium")) {
        // setup premium connection page
    }
}

void MainWindow::loadSetting()
{
    ui->label_version->setText(QString("v.1.0, %1").arg(__DATE__));

    // General
    ui->checkBox_LaunchAtStart->setChecked(Settings::launchOnStartup());
    ui->checkBox_ConnectWhenStarted->setChecked(Settings::connectWhenStarted());
    ui->checkBox_ReconnectIfDrop->setChecked(Settings::reconnectOnDrop());
    ui->checkBox_KillSwitch->setChecked(Settings::networkLock());
}


void MainWindow::setSettingsConnections()
{
    // General
    connect(ui->checkBox_LaunchAtStart, &QCheckBox::toggled, [&](){
        Settings::setLaunchOnStartup(ui->checkBox_LaunchAtStart->isChecked());

#ifdef Q_OS_WIN
        winhelpLaunchStartup("Obhod123", ui->checkBox_LaunchAtStart->isChecked(), NULL);
#endif
    });

    connect(ui->checkBox_ConnectWhenStarted, &QCheckBox::toggled, [&](){
        Settings::setConnectWhenStarted(ui->checkBox_ConnectWhenStarted->isChecked());
    });

    connect(ui->checkBox_ReconnectIfDrop, &QCheckBox::toggled, [&](){
        Settings::setReconnectOnDrop(ui->checkBox_ReconnectIfDrop->isChecked());
    });

    connect(ui->checkBox_KillSwitch, &QCheckBox::toggled, [&](){
        Settings::setNetworkLock(ui->checkBox_KillSwitch->isChecked());
    });

//    connect(ui->checkBox_accessToNetDevices, &QCheckBox::clicked, [&](){
//        q_settings.setValue("accessToNetDevices", ui->checkBox_accessToNetDevices->isChecked());
    //    });
}


