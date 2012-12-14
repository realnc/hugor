#include <QSettings>
#include <QDebug>
#include <QFileInfo>

#include "settings.h"
#include "hmainwindow.h"
#include "hugodefs.h"
extern "C" {
#include "heheader.h"
}


#if QT_VERSION < 0x040700
static int
qtRuntimeVersion()
{
    const QList<QByteArray> verList(QByteArray(qVersion()).split('.'));
    if (verList.size() < 3) {
        // Something isn't right. The Qt version string should have
        // at least three fields.
        return 0;
    }
    bool ok;
    int major = verList.at(0).toInt(&ok);
    if (not ok) {
        return 0;
    }
    int minor = verList.at(1).toInt(&ok);
    if (not ok) {
        return 0;
    }
    int patch = verList.at(2).toInt(&ok);
    if (not ok) {
        return 0;
    }
    return QT_VERSION_CHECK(major, minor, patch);
}
#endif


void
Settings::loadFromDisk()
{
    QSettings sett;

    sett.beginGroup(QString::fromLatin1("media"));
    this->enableGraphics = sett.value(QString::fromLatin1("graphics"), true).toBool();
#ifndef Q_WS_ANDROID
    this->enableSoundEffects = sett.value(QString::fromLatin1("sounds"), true).toBool();
    this->enableMusic = sett.value(QString::fromLatin1("music"), true).toBool();
#else
    this->enableSoundEffects = sett.value(QString::fromLatin1("sounds"), false).toBool();
    this->enableMusic = sett.value(QString::fromLatin1("music"), false).toBool();
#endif
    this->useSmoothScaling = sett.value(QString::fromLatin1("smoothImageScaling"), true).toBool();
    this->muteSoundInBackground = sett.value(QString::fromLatin1("pauseSoundInBackground"), true).toBool();
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("colors"));
    this->mainBgColor = sett.value(QString::fromLatin1("mainbg"), hugoColorToQt(DEF_BGCOLOR)).value<QColor>();
    this->mainTextColor = sett.value(QString::fromLatin1("maintext"), hugoColorToQt(DEF_FCOLOR)).value<QColor>();
    this->statusBgColor = sett.value(QString::fromLatin1("bannerbg"), hugoColorToQt(DEF_SLBGCOLOR)).value<QColor>();
    this->statusTextColor = sett.value(QString::fromLatin1("bannertext"), hugoColorToQt(DEF_SLFCOLOR)).value<QColor>();
    sett.endGroup();

#ifdef Q_WS_MAC
    const QString& DEFAULT_PROP = QString::fromLatin1("Georgia,15");
    const QString& DEFAULT_MONO = QString::fromLatin1("Andale Mono,15");
#else
#ifdef Q_WS_WIN
    const QString& DEFAULT_PROP = QString::fromLatin1("Times New Roman,12");
    const QString& DEFAULT_MONO = QString::fromLatin1("Courier New,12");
#else
#ifdef Q_WS_ANDROID
    const QString& DEFAULT_PROP = QString::fromLatin1("Droid Serif");
    const QString& DEFAULT_MONO = QString::fromLatin1("Droid Sans Mono");
#else
    const QString& DEFAULT_PROP = QString::fromLatin1("serif");
    const QString& DEFAULT_MONO = QString::fromLatin1("monospace");
#endif
#endif
#endif
    sett.beginGroup(QString::fromLatin1("fonts"));
    QFont::StyleStrategy strat;
#if QT_VERSION >= 0x040700
    // We're building with a recent enough Qt; use ForceIntegerMetrics directly.
    strat = QFont::StyleStrategy(QFont::PreferOutline | QFont::PreferQuality | QFont::ForceIntegerMetrics);
#else
    // We're building with a Qt version that does not offer ForceIntegerMetrics.
    // If we're running on a recent enough Qt, use the ForceIntegerMetrics enum
    // value directly.
    if (qtRuntimeVersion() >= 0x040700) {
        strat = QFont::StyleStrategy(QFont::PreferOutline | QFont::PreferQuality | 0x0400);
    } else {
        strat = QFont::StyleStrategy(QFont::PreferOutline | QFont::PreferQuality);
    }
#endif
    this->propFont.setStyleStrategy(strat);
    QFont tmp;
    tmp.fromString(sett.value(QString::fromLatin1("main"), DEFAULT_PROP).toString());
    this->propFont.setFamily(tmp.family());
    this->propFont.setPointSize(tmp.pointSize());
    this->fixedFont.setStyleStrategy(strat);
    tmp.fromString(sett.value(QString::fromLatin1("fixed"), DEFAULT_MONO).toString());
    this->fixedFont.setFamily(tmp.family());
    this->fixedFont.setPointSize(tmp.pointSize());
    this->softTextScrolling = sett.value(QString::fromLatin1("softTextScrolling"), true).toBool();
    this->extraButter = sett.value(QString::fromLatin1("extraButter"), false).toBool();
    this->smartFormatting = sett.value(QString::fromLatin1("smartFormatting"), true).toBool();
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("misc"));
    this->askForGameFile = sett.value(QString::fromLatin1("askforfileatstart"), true).toBool();
    this->lastFileOpenDir = sett.value(QString::fromLatin1("lastFileOpenDir"), QString::fromLatin1("")).toString();
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("recent"));
    this->recentGamesList = sett.value(QString::fromLatin1("games"), QStringList()).toStringList();
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

    sett.beginGroup(QString::fromLatin1("geometry"));
    this->appSize = sett.value(QString::fromLatin1("size"), QSize(800, 600)).toSize();
    this->overlayScrollback = sett.value(QString::fromLatin1("overlayScrollback"), true).toBool();
    this->isMaximized = sett.value(QString::fromLatin1("maximized"), false).toBool();
    this->marginSize = sett.value(QString::fromLatin1("marginSize"), 0).toInt();
    this->fullscreenWidth = sett.value(QString::fromLatin1("fullscreenWidth"), 0).toInt();
    sett.endGroup();
}


void
Settings::saveToDisk()
{
    QSettings sett;

    sett.beginGroup(QString::fromLatin1("media"));
    sett.setValue(QString::fromLatin1("graphics"), this->enableGraphics);
    sett.setValue(QString::fromLatin1("sounds"), this->enableSoundEffects);
    sett.setValue(QString::fromLatin1("music"), this->enableMusic);
    sett.setValue(QString::fromLatin1("smoothImageScaling"), this->useSmoothScaling);
    sett.setValue(QString::fromLatin1("pauseSoundInBackground"), this->muteSoundInBackground);
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("colors"));
    sett.setValue(QString::fromLatin1("mainbg"), this->mainBgColor);
    sett.setValue(QString::fromLatin1("maintext"), this->mainTextColor);
    sett.setValue(QString::fromLatin1("bannerbg"), this->statusBgColor);
    sett.setValue(QString::fromLatin1("bannertext"), this->statusTextColor);
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("fonts"));
    sett.setValue(QString::fromLatin1("main"), this->propFont.toString());
    sett.setValue(QString::fromLatin1("fixed"), this->fixedFont.toString());
    sett.setValue(QString::fromLatin1("softTextScrolling"), this->softTextScrolling);
    sett.setValue(QString::fromLatin1("extraButter"), this->extraButter);
    sett.setValue(QString::fromLatin1("smartFormatting"), this->smartFormatting);
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("misc"));
    sett.setValue(QString::fromLatin1("askforfileatstart"), this->askForGameFile);
    sett.setValue(QString::fromLatin1("lastFileOpenDir"), this->lastFileOpenDir);
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("recent"));
    sett.setValue(QString::fromLatin1("games"), this->recentGamesList);
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("geometry"));
    // Do not save application size if we're in fullscreen mode, since we
    // need to restore the windowed, non-fullscreen size next time we run.
    if (not hMainWin->isFullScreen()) {
        sett.setValue(QString::fromLatin1("size"), hMainWin->size());
    }
    sett.setValue(QString::fromLatin1("overlayScrollback"), this->overlayScrollback);
    sett.setValue(QString::fromLatin1("maximized"), hMainWin->isMaximized());
    sett.setValue(QString::fromLatin1("marginSize"), this->marginSize);
    sett.setValue(QString::fromLatin1("fullscreenWidth"), this->fullscreenWidth);
    sett.endGroup();
    sett.sync();
}
