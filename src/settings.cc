#include <QSettings>
#include <QDebug>
#include <QFileInfo>

#include "settings.h"
#include "hmainwindow.h"
#include "hugodefs.h"
extern "C" {
#include "heheader.h"
}


void
Settings::loadFromDisk()
{
    QSettings sett;

    sett.beginGroup(QString::fromAscii("media"));
    this->enableGraphics = sett.value(QString::fromAscii("graphics"), true).toBool();
#ifndef Q_WS_ANDROID
    this->enableSoundEffects = sett.value(QString::fromAscii("sounds"), true).toBool();
    this->enableMusic = sett.value(QString::fromAscii("music"), true).toBool();
#else
    this->enableSoundEffects = sett.value(QString::fromAscii("sounds"), false).toBool();
    this->enableMusic = sett.value(QString::fromAscii("music"), false).toBool();
#endif
    this->useSmoothScaling = sett.value(QString::fromAscii("smoothImageScaling"), true).toBool();
    sett.endGroup();

    sett.beginGroup(QString::fromAscii("colors"));
    this->mainBgColor = sett.value(QString::fromAscii("mainbg"), hugoColorToQt(DEF_BGCOLOR)).value<QColor>();
    this->mainTextColor = sett.value(QString::fromAscii("maintext"), hugoColorToQt(DEF_FCOLOR)).value<QColor>();
    this->statusBgColor = sett.value(QString::fromAscii("bannerbg"), hugoColorToQt(DEF_SLBGCOLOR)).value<QColor>();
    this->statusTextColor = sett.value(QString::fromAscii("bannertext"), hugoColorToQt(DEF_SLFCOLOR)).value<QColor>();
    sett.endGroup();

#ifdef Q_WS_MAC
    const QString& DEFAULT_PROP = QString::fromAscii("Georgia,15");
    const QString& DEFAULT_MONO = QString::fromAscii("Andale Mono,15");
#else
#ifdef Q_WS_WIN
    const QString& DEFAULT_PROP = QString::fromAscii("Times New Roman,12");
    const QString& DEFAULT_MONO = QString::fromAscii("Courier New,12");
#else
#ifdef Q_WS_ANDROID
    const QString& DEFAULT_PROP = QString::fromAscii("Droid Serif");
    const QString& DEFAULT_MONO = QString::fromAscii("Droid Sans Mono");
#else
    const QString& DEFAULT_PROP = QString::fromAscii("serif");
    const QString& DEFAULT_MONO = QString::fromAscii("monospace");
#endif
#endif
#endif
    sett.beginGroup(QString::fromAscii("fonts"));
    this->propFont.fromString(sett.value(QString::fromAscii("main"), DEFAULT_PROP).toString());
    this->propFont.setKerning(true);
    this->fixedFont.fromString(sett.value(QString::fromAscii("fixed"), DEFAULT_MONO).toString());
    this->fixedFont.setKerning(true);
    sett.endGroup();

    sett.beginGroup(QString::fromAscii("misc"));
    this->askForGameFile = sett.value(QString::fromAscii("askforfileatstart"), true).toBool();
    this->lastFileOpenDir = sett.value(QString::fromAscii("lastFileOpenDir"), QString::fromAscii("")).toString();
    sett.endGroup();

    sett.beginGroup(QString::fromAscii("recent"));
    this->recentGamesList = sett.value(QString::fromAscii("games"), QStringList()).toStringList();
    Q_ASSERT(this->recentGamesList.size() <= this->recentGamesCapacity);
    // Remove any files that don't exist or aren't readable.
    for (int i = 0; i < this->recentGamesList.size(); ++i) {
        QFileInfo file(this->recentGamesList.at(i));
        if (not file.exists() or not (file.isFile() or file.isSymLink()) or not file.isReadable()) {
            this->recentGamesList.removeAt(i);
            --i;
        }
    }
    sett.endGroup();

    this->appSize = sett.value(QString::fromAscii("geometry/size"), QSize(800, 600)).toSize();
}


void
Settings::saveToDisk()
{
    QSettings sett;

    sett.beginGroup(QString::fromAscii("media"));
    sett.setValue(QString::fromAscii("graphics"), this->enableGraphics);
    sett.setValue(QString::fromAscii("sounds"), this->enableSoundEffects);
    sett.setValue(QString::fromAscii("music"), this->enableMusic);
    sett.setValue(QString::fromAscii("smoothImageScaling"), this->useSmoothScaling);
    sett.endGroup();

    sett.beginGroup(QString::fromAscii("colors"));
    sett.setValue(QString::fromAscii("mainbg"), this->mainBgColor);
    sett.setValue(QString::fromAscii("maintext"), this->mainTextColor);
    sett.setValue(QString::fromAscii("bannerbg"), this->statusBgColor);
    sett.setValue(QString::fromAscii("bannertext"), this->statusTextColor);
    sett.endGroup();

    sett.beginGroup(QString::fromAscii("fonts"));
    sett.setValue(QString::fromAscii("main"), this->propFont.toString());
    sett.setValue(QString::fromAscii("fixed"), this->fixedFont.toString());
    sett.endGroup();

    sett.beginGroup(QString::fromAscii("misc"));
    sett.setValue(QString::fromAscii("askforfileatstart"), this->askForGameFile);
    sett.setValue(QString::fromAscii("lastFileOpenDir"), this->lastFileOpenDir);
    sett.endGroup();

    sett.beginGroup(QString::fromAscii("recent"));
    sett.setValue(QString::fromAscii("games"), this->recentGamesList);
    sett.endGroup();

    sett.setValue(QString::fromAscii("geometry/size"), hMainWin->size());
    sett.sync();
}
