#include <QGuiApplication>
#include <QFontDatabase>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QFile>
#include <QTranslator>
#include <QLibraryInfo>
#include <QtWebView/QtWebView>

#include "publib/runguard.h"
#include "backend/debug.h"

#include "ui/mainwindow.h"

#ifdef Q_OS_WIN
#include "publib/winhelp.h"
#include <windows.h>
#endif

#define ApplicationName "Obhod123"


int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(res);

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    RunGuard::instance(ApplicationName).activate();

    QApplication app(argc, argv);
    //QtWebView::initialize();

    if (! RunGuard::instance().tryToRun()) {
        qDebug() << "Tried to run second instance. Exiting...";
        QMessageBox::information(NULL, QObject::tr("Notify"), QObject::tr("Obhod123 is already running."));
        return 0;
    }

    QFontDatabase::addApplicationFont(QString(":/fonts/OpenSans-Regular.ttf"));
    QFontDatabase::addApplicationFont(QString(":/fonts/OpenSans-Bold.ttf"));
    QFontDatabase::addApplicationFont(QString(":/fonts/OpenSans-Semibold.ttf"));
    QFontDatabase::addApplicationFont(QString(":/fonts/Ancient-Medium.ttf"));
    QFontDatabase::addApplicationFont(QString(":/fonts/JMH Cthulhumbus.otf"));

    {
        QTranslator *translator = new QTranslator;
        QLocale ru(QLocale("ru_RU"));
        QLocale::setDefault(ru);
        if (translator->load(QLocale(), "obhod123", ".", QLatin1String(":/translations"))) {
            bool ok = qApp->installTranslator(translator);
            qDebug().noquote() << "Main: Installing translator for locale" << ru.name() << ok;
        }
        else {
            qDebug().noquote() << "Main: Failed to install translator for locale" << ru.name();
        }
    }


    app.setOrganizationName("obhod123");
    app.setOrganizationDomain("obhod123.com");
    app.setApplicationName(ApplicationName);
    app.setApplicationDisplayName(ApplicationName);
    app.setApplicationVersion("1.0.0.0");

    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription("WTF? What you are looking for?");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption debugToConsoleOption("d", QCoreApplication::translate("main", "Output to console instead log file"));
    parser.addOption(debugToConsoleOption);

#ifdef Q_OS_MAC
    QCommandLineOption forceUseBrightIconsOption("b", QCoreApplication::translate("main", "Force use bright icons"));
    parser.addOption(forceUseBrightIconsOption);
#endif

    // Process the actual command line arguments given by the user
    parser.process(app);

    bool debugToConsole = parser.isSet(debugToConsoleOption);
    bool forceUseBrightIcons = false;

#ifdef Q_OS_MAC
    forceUseBrightIcons = parser.isSet(forceUseBrightIconsOption);
#endif


        qDebug() << "Set output to console: " << debugToConsole;
    if (!debugToConsole) {
        if (!Debug::init()) {
            qCritical() << "Initialization of debug subsystem failed";
        }
    }

    MainWindow mainWindow(forceUseBrightIcons);
    mainWindow.show();

    QFont f("Open Sans Regular", 10);
    f.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(f);

    return app.exec();
}
