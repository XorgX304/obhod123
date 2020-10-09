#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>
#include <QJsonDocument>
#include <QClipboard>
#include <QStringListModel>
#include <QDataStream>

#include <QGraphicsBlurEffect>
#include "customshadoweffect.h"

#include "../backend/obhod123.h"
#include "../backend/license.h"

#include "../backend/networkcontroller_win.h"

#define DM_APP_KEY "0000"


namespace Ui {
class MainWindow;
}

/**
 * @brief The MainWindow class - Main application window
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(bool setForceUseBrightIcons = false, QWidget *parent = 0);
    ~MainWindow();
    //bool eventFilter(QObject *obj, QEvent *event) override;

    QString secondsToString(qint64 seconds);

    enum LastAction {Connect, CancelConnect, Disconnect};

    bool eventFilter(QObject *obj, QEvent *event) override;
public slots:
    void on_pushButton_hide_clicked();
    void on_pushButton_close_clicked();

    void on_pushButton_login_clicked();

    void on_pushButton_cancel_clicked();

    void on_checkBox_lauchOnStartup_clicked();

    // connect buttons
    void on_pushButton_connect_clicked();
    void on_pushButton_cancelConnect_clicked();
    void on_pushButton_disconnect_clicked();

    void onVpnStateChanged(VPNClient::STATE state);
    void onVpnMessage(QString msg);
    void onTrayActionConnect(); // connect in context menu
    void onSpeedCounterRequest();
    void onSpeedCounterRefresh();

    void setupInitPage();
    void setupObhodPage();

    void mayBeAutoLogin();

    void showObhod123();
    void showAnimated();
    void hideAnimated();

private slots:
    void on_pushButton_logout_clicked();

    void on_lineEdit_username_returnPressed();

    void on_pushButton_blocked_list_clicked();
    void on_pushButton_back_from_sites_clicked();

    void on_pushButton_sites_add_custom_clicked();
    void on_pushButton_sites_delete_custom_clicked();

    void on_pushButton_back_from_after_update_clicked();

    void on_pushButton_new_server_clicked();

    void on_pushButton_settings_clicked();

private:
    void onConnect();
    void onDisconnect();
    void onCancelConnect();

    void closeEvent(QCloseEvent *event) override;

    void setVpnConnections();
    /**
     * @brief setTrayIcon - setup tray icon with size 128x128, on macOS it also depends from the taskbar theme
     * @param iconName - icon name
     */
    void setTrayIcon(const QString& iconName);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void setupTray();
    void checkIfFirstRun();

    Ui::MainWindow *ui;

    QPoint initialMovePoint;
    bool canMove = false;

    QTimer durationTimer;
    QTimer speedCounterRequestTimer;
    QTimer speedCounterRefreshTimer;

    Obhod123 obhod123;

    QSystemTrayIcon m_tray;
    QMenu* m_menu;
    QAction* m_trayActionConnect;
    qint64 connectedDateTime;
    bool forceUseBrightIcons;


    void initBlockedSites();
    void initCustomSites();

    // traffic counters updated every 1 second
    // we interpolate counters every 10 msec to make counters smooth
    qint64  m_receiveSpeedPrev, m_sendSpeedPrev, m_receiveSpeedNext, m_sendSpeedNext;


    bool needAutoLogin;
    bool needExit; // used for exiting from Event loops

    LastAction lastUserAction;   // last button pressed by user

    void setupGraphicsEffects();

    // Settings
    void loadSetting();
    void setSettingsConnections();

    QStringListModel *customSitesModel;
};

#endif // MAINWINDOW_H
