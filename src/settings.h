#ifndef SETTINGS_H
#define SETTINGS_H

#include <QFont>
#include <QColor>
#include <QSize>


class Settings {
  public:
    void
    loadFromDisk( class SettingsOverrides* ovr = 0 );

    void
    saveToDisk();

    bool enableGraphics;
    bool enableSoundEffects;
    bool enableMusic;
    bool useSmoothScaling;
    bool muteSoundInBackground;

    QColor mainTextColor;
    QColor mainBgColor;
    QColor statusTextColor;
    QColor statusBgColor;

    QFont propFont;
    QFont fixedFont;
    bool softTextScrolling;
    bool extraButter;
    bool smartFormatting;

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
};


#endif
