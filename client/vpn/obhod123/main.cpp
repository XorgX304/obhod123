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

    QFontDatabase::addApplicationFont(":/fonts/Lato-Black.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-BlackItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-BoldItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Italic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Light.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-LightItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Thin.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-ThinItalic.ttf");


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


    app.setOrganizationName("VPN123");
    app.setOrganizationDomain("VPN123.ORG");
    app.setApplicationName(ApplicationName);
    app.setApplicationDisplayName(ApplicationName);
    app.setApplicationVersion("1.0.0.0");

    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription("VPN123");
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

    QFont f("Lato Regular", 10);
    f.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(f);

    return app.exec();
}
