#ifndef SETTINGS_H
#define SETTINGS_H

#include <QFont>
#include <QColor>
#include <QSize>


class Settings {
  public:
    Settings()
        : videoSysError(false),
          widthRatio(4),
          heightRatio(3)
    { }

    void
    loadFromDisk( class SettingsOverrides* ovr = 0 );

    void
    saveToDisk();

    bool enableGraphics;
    bool enableVideo;
    bool enableSoundEffects;
    bool enableMusic;
    bool useSmoothScaling;
    bool muteWhenMinimized;
    int soundVolume;

    QColor mainTextColor;
    QColor mainBgColor;
    QColor statusTextColor;
    QColor statusBgColor;
    bool customFsMarginColor;
    QColor fsMarginColor;

    QFont propFont;
    QFont fixedFont;
    QFont scrollbackFont;
    bool softTextScrolling;
    bool smartFormatting;
    int scriptWrap;

    bool askForGameFile;
    QString lastFileOpenDir;

    QStringList recentGamesList;
    static const int recentGamesCapacity = 10;

    QSize appSize;
    bool isMaximized;
    bool isFullscreen;
    bool overlayScrollback;
    int marginSize;
    int fullscreenWidth;

    // These are not saved. Used for temporary overrides that only apply
    // to the current session.
    bool videoSysError;
    int widthRatio;
    int heightRatio;
};


#endif
