#include "settingsoverrides.h"

#include <QSettings>


SettingsOverrides::SettingsOverrides( const QString& filename )
{
    fSett = new QSettings(filename, QSettings::IniFormat);
    fSett->beginGroup(QString::fromLatin1("display"));
    this->appName = fSett->value(QString::fromLatin1("appName"), QString::fromLatin1("")).toString();
    this->authorName = fSett->value(QString::fromLatin1("authorName"), QString::fromLatin1("")).toString();
    this->fullscreen = fSett->value(QString::fromLatin1("fullscreen"), false).toBool();
    this->hideMenuBar = fSett->value(QString::fromLatin1("hideMenuBar"), false).toBool();
    this->fullscreenWidth = fSett->value(QString::fromLatin1("fullscreenWidth"), 0).toInt();
    fSett->endGroup();
}
