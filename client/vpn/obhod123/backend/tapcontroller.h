#ifndef TAPCONTROLLER_H
#define TAPCONTROLLER_H

#include <QObject>
#include <QApplication>
#include <QProcess>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QSettings>


#include "../publib/winhelp.h"

class TapController : public QObject
{
    Q_OBJECT
public:
    static TapController& Instance();

    bool checkInstaller();
    bool setupDriver();

    bool removeDriver(QString tapInstanceId);
signals:

public slots:

private:
    explicit TapController(QObject *parent = 0);
    TapController(TapController const &) = delete;
    TapController& operator= (TapController const&) = delete;

    QStringList getTapList();  // return list of instance id

    QString tapDriversDirPath;
    QString tapInstallDirPath;
    QString tapName;
    void setTapInfo();

    bool checkDriver(QString tapInstanceId);

    QSettings s;

};

#endif // TAPCONTROLLER_H
