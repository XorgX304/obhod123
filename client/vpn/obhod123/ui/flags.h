#ifndef FLAGS_H
#define FLAGS_H

#include <QObject>
#include <QHash>
#include <QImageReader>

/**
 * @brief The Flags class - Image loader for flags
 */
class Flags : public QObject
{
    Q_OBJECT
public:
    static Flags& Instance();
    const QImage& getFlag(const QString& country);

private:
    Flags();
    Flags(Flags const &) = delete;
    Flags& operator= (Flags const&) = delete;

    QHash<QString, QImage> flags;
};

#endif // FLAGS_H
