#include "settings.h"

QSettings &Settings::q_settings()
{
    static QSettings q_settings;
    return q_settings;
}




