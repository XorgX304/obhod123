#include "flags.h"

Flags::Flags()
{

}

Flags &Flags::Instance()
{
    static Flags s;
    return s;
}

const QImage &Flags::getFlag(const QString& country)
{
#ifdef Q_OS_WIN
    if (flags.contains(country.toUpper())) return flags[country.toUpper()];

    QImageReader ir(QString(":/images/flags/%1.png").arg(country.toUpper()));
    QImage flag = ir.read();
    flags.insert(country.toUpper(), flag);
    return flags[country.toUpper()];
#endif

#ifdef Q_OS_MAC
    if (flags.contains(country.toLower())) return flags[country.toLower()];

    QImageReader ir(QString(":/images_mac/flags/%1.png").arg(country.toLower()));
    QImage flag = ir.read();
    flags.insert(country.toLower(), flag.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    return flags[country.toLower()];
#endif
}
