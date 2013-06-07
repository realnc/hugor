#include <QSettings>
#include <QDebug>
#include <QFileInfo>
#include <QApplication>
#include <QDesktopWidget>

#include "settings.h"
#include "hmainwindow.h"
#include "hugodefs.h"
extern "C" {
#include "heheader.h"
}
#include "settingsoverrides.h"


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


#define SETT_MEDIA_GRP QString::fromLatin1("media")
#define SETT_COLORS_GRP QString::fromLatin1("colors")
#define SETT_FONTS_GRP QString::fromLatin1("fonts")
#define SETT_RECENT_GRP QString::fromLatin1("recent")
#define SETT_MISC_GRP QString::fromLatin1("misc")
#define SETT_GEOMETRY_GRP QString::fromLatin1("geometry")
#define SETT_GRAPHICS QString::fromLatin1("graphics")
#define SETT_VIDEO QString::fromLatin1("video")
#define SETT_SOUNDS QString::fromLatin1("sounds")
#define SETT_MUSIC QString::fromLatin1("music")
#define SETT_SMOOTH_IMAGES QString::fromLatin1("smoothImageScaling")
#define SETT_PAUSE_SOUND QString::fromLatin1("pauseSoundInBackground")
#define SETT_MAIN_BG_COLOR QString::fromLatin1("mainbg")
#define SETT_MAIN_TXT_COLOR QString::fromLatin1("maintext")
#define SETT_STATUS_BG_COLOR QString::fromLatin1("bannerbg")
#define SETT_STATUS_TXT_COLOR QString::fromLatin1("bannertext")
#define SETT_MAIN_FONT QString::fromLatin1("main")
#define SETT_FIXED_FONT QString::fromLatin1("fixed")
#define SETT_SOFT_SCROLL QString::fromLatin1("softTextScrolling")
#define SETT_EXTRA_BUTTER QString::fromLatin1("extraButter")
#define SETT_SMART_FORMATTING QString::fromLatin1("smartFormatting")
#define SETT_ASK_FILE QString::fromLatin1("askforfileatstart")
#define SETT_LAST_OPEN_DIR QString::fromLatin1("lastFileOpenDir")
#define SETT_GAMES_LIST QString::fromLatin1("games")
#define SETT_APP_SIZE QString::fromLatin1("size")
#define SETT_OVERLAY_SCROLL QString::fromLatin1("overlayScrollback")
#define SETT_MAXIMIZED QString::fromLatin1("maximized")
#define SETT_FULLSCREEN QString::fromLatin1("fullscreen")
#define SETT_MARGIN_SIZE QString::fromLatin1("marginSize")
#define SETT_FULLSCREEN_WIDTH QString::fromLatin1("fullscreenWidth")


void
Settings::loadFromDisk( SettingsOverrides* ovr )
{
    QSettings sett;

    sett.beginGroup(SETT_MEDIA_GRP);
    this->enableGraphics = sett.value(SETT_GRAPHICS, true).toBool();
    this->enableVideo = sett.value(SETT_VIDEO, true).toBool();
#ifndef Q_WS_ANDROID
    this->enableSoundEffects = sett.value(SETT_SOUNDS, true).toBool();
    this->enableMusic = sett.value(SETT_MUSIC, true).toBool();
#else
    this->enableSoundEffects = sett.value(SETT_SOUNDS, false).toBool();
    this->enableMusic = sett.value(SETT_MUSIC, false).toBool();
#endif
    this->useSmoothScaling = sett.value(SETT_SMOOTH_IMAGES, true).toBool();
    this->muteSoundInBackground = sett.value(SETT_PAUSE_SOUND, true).toBool();
    sett.endGroup();

    sett.beginGroup(SETT_COLORS_GRP);
    this->mainBgColor = sett.value(SETT_MAIN_BG_COLOR, hugoColorToQt(DEF_BGCOLOR)).value<QColor>();
    this->mainTextColor = sett.value(SETT_MAIN_TXT_COLOR, hugoColorToQt(DEF_FCOLOR)).value<QColor>();
    this->statusBgColor = sett.value(SETT_STATUS_BG_COLOR, hugoColorToQt(DEF_SLBGCOLOR)).value<QColor>();
    this->statusTextColor = sett.value(SETT_STATUS_TXT_COLOR, hugoColorToQt(DEF_SLFCOLOR)).value<QColor>();
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
    sett.beginGroup(SETT_FONTS_GRP);
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
    tmp.fromString(sett.value(SETT_MAIN_FONT, DEFAULT_PROP).toString());
    this->propFont.setFamily(tmp.family());
    this->propFont.setPointSize(tmp.pointSize());
    this->fixedFont.setStyleStrategy(strat);
    tmp.fromString(sett.value(SETT_FIXED_FONT, DEFAULT_MONO).toString());
    this->fixedFont.setFamily(tmp.family());
    this->fixedFont.setPointSize(tmp.pointSize());
    this->softTextScrolling = sett.value(SETT_SOFT_SCROLL, true).toBool();
    this->extraButter = sett.value(SETT_EXTRA_BUTTER, false).toBool();
    this->smartFormatting = sett.value(SETT_SMART_FORMATTING, true).toBool();
    sett.endGroup();

    sett.beginGroup(SETT_MISC_GRP);
    this->askForGameFile = sett.value(SETT_ASK_FILE, true).toBool();
    this->lastFileOpenDir = sett.value(SETT_LAST_OPEN_DIR, QString::fromLatin1("")).toString();
    sett.endGroup();

    sett.beginGroup(SETT_RECENT_GRP);
    this->recentGamesList = sett.value(SETT_GAMES_LIST, QStringList()).toStringList();
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

    sett.beginGroup(SETT_GEOMETRY_GRP);
    this->appSize = sett.value(SETT_APP_SIZE, QSize(800, 600)).toSize();
    this->overlayScrollback = sett.value(SETT_OVERLAY_SCROLL, true).toBool();
    this->isMaximized = sett.value(SETT_MAXIMIZED, false).toBool();
    this->isFullscreen = sett.value(SETT_FULLSCREEN, false).toBool();
    this->marginSize = sett.value(SETT_MARGIN_SIZE, 0).toInt();
    // If fullscreen width is not set, use one that results in a 4:3 ratio.
    int scrWidth = QApplication::desktop()->screenGeometry().width();
    this->fullscreenWidth = sett.value(SETT_FULLSCREEN_WIDTH,
                                       (double)scrWidth / (4.0 / 3.0)).toInt();
    sett.endGroup();

    // Apply overrides for non-existent settings.
    if (ovr) {
        sett.beginGroup(SETT_GEOMETRY_GRP);
        if (not sett.contains(SETT_FULLSCREEN_WIDTH)) {
            this->fullscreenWidth = ovr->fullscreenWidth;
        }
        if (not sett.contains(SETT_MARGIN_SIZE)) {
            this->marginSize = ovr->marginSize;
        }
        sett.endGroup();

        sett.beginGroup(SETT_FONTS_GRP);
        if (not sett.contains(SETT_MAIN_FONT)) {
            this->propFont.setPointSize(ovr->propFontSize);
        }
        if (not sett.contains(SETT_FIXED_FONT)) {
            this->fixedFont.setPointSize(ovr->fixedFontSize);
        }
        sett.endGroup();

        sett.beginGroup(SETT_MEDIA_GRP);
        if (not sett.contains(SETT_SMOOTH_IMAGES)) {
            this->useSmoothScaling = ovr->imageSmoothing;
        }
        if (not sett.contains(SETT_PAUSE_SOUND)) {
            this->muteSoundInBackground = ovr->pauseAudio;
        }
        sett.endGroup();
    }
}


void
Settings::saveToDisk()
{
    QSettings sett;

    sett.beginGroup(SETT_MEDIA_GRP);
    sett.setValue(SETT_GRAPHICS, this->enableGraphics);
    sett.setValue(SETT_VIDEO, this->enableVideo);
    sett.setValue(SETT_SOUNDS, this->enableSoundEffects);
    sett.setValue(SETT_MUSIC, this->enableMusic);
    sett.setValue(SETT_SMOOTH_IMAGES, this->useSmoothScaling);
    sett.setValue(SETT_PAUSE_SOUND, this->muteSoundInBackground);
    sett.endGroup();

    sett.beginGroup(SETT_COLORS_GRP);
    sett.setValue(SETT_MAIN_BG_COLOR, this->mainBgColor);
    sett.setValue(SETT_MAIN_TXT_COLOR, this->mainTextColor);
    sett.setValue(SETT_STATUS_BG_COLOR, this->statusBgColor);
    sett.setValue(SETT_STATUS_TXT_COLOR, this->statusTextColor);
    sett.endGroup();

    sett.beginGroup(SETT_FONTS_GRP);
    sett.setValue(SETT_MAIN_FONT, this->propFont.toString());
    sett.setValue(SETT_FIXED_FONT, this->fixedFont.toString());
    sett.setValue(SETT_SOFT_SCROLL, this->softTextScrolling);
    sett.setValue(SETT_EXTRA_BUTTER, this->extraButter);
    sett.setValue(SETT_SMART_FORMATTING, this->smartFormatting);
    sett.endGroup();

    sett.beginGroup(SETT_MISC_GRP);
    sett.setValue(SETT_ASK_FILE, this->askForGameFile);
    sett.setValue(SETT_LAST_OPEN_DIR, this->lastFileOpenDir);
    sett.endGroup();

    sett.beginGroup(SETT_RECENT_GRP);
    sett.setValue(SETT_GAMES_LIST, this->recentGamesList);
    sett.endGroup();

    sett.beginGroup(SETT_GEOMETRY_GRP);
    // Do not save application size if we're in fullscreen mode, since we
    // need to restore the windowed, non-fullscreen size next time we run.
    if (not hMainWin->isFullScreen()) {
        sett.setValue(SETT_APP_SIZE, hMainWin->size());
    }
    sett.setValue(SETT_OVERLAY_SCROLL, this->overlayScrollback);
    sett.setValue(SETT_MAXIMIZED, hMainWin->isMaximized());
    sett.setValue(SETT_FULLSCREEN, hMainWin->isFullScreen());
    sett.setValue(SETT_MARGIN_SIZE, this->marginSize);
    sett.setValue(SETT_FULLSCREEN_WIDTH, this->fullscreenWidth);
    sett.endGroup();
    sett.sync();
}
