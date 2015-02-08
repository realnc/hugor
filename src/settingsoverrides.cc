#include "settingsoverrides.h"

#include <QSettings>


SettingsOverrides::SettingsOverrides( const QString& filename )
{
    QSettings sett(filename, QSettings::IniFormat);

    sett.beginGroup(QString::fromLatin1("identity"));
    this->appName = sett.value(QString::fromLatin1("appName"), QString::fromLatin1("")).toString();
    this->authorName = sett.value(QString::fromLatin1("authorName"), QString::fromLatin1("")).toString();
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("display"));
    this->fullscreen = sett.value(QString::fromLatin1("fullscreen"), false).toBool();
    this->hideMenuBar = sett.value(QString::fromLatin1("hideMenuBar"), false).toBool();
    this->fullscreenWidth = sett.value(QString::fromLatin1("fullscreenWidth"), 100).toInt();
    if (this->fullscreenWidth > 100) {
        this->fullscreenWidth = 100;
    } else if (this->fullscreenWidth < 10) {
        this->fullscreenWidth = 10;
    }
    this->marginSize = sett.value(QString::fromLatin1("marginSize"), 0).toInt();
    this->widthRatio = sett.value(QString::fromLatin1("widthRatio"), 4).toInt();
    this->heightRatio = sett.value(QString::fromLatin1("heightRatio"), 3).toInt();
    this->propFontSize = sett.value(QString::fromLatin1("propFontSize"), 0).toInt();
    this->fixedFontSize = sett.value(QString::fromLatin1("fixedFontSize"), 0).toInt();
    this->imageSmoothing = sett.value(QString::fromLatin1("imageSmoothing"), true).toBool();
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("media"));
    this->pauseAudio = sett.value(QString::fromLatin1("backgroundPauseAudio"), true).toBool();
    sett.endGroup();
}
