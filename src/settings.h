#ifndef SETTINGS_H
#define SETTINGS_H

#include <QFont>
#include <QColor>
#include <QSize>


class Settings {
  public:
    void
    loadFromDisk();

    void
    saveToDisk();

    bool enableGraphics;
    bool enableSoundEffects;
    bool enableMusic;
    bool useSmoothScaling;

    QColor mainTextColor;
    QColor mainBgColor;
    QColor statusTextColor;
    QColor statusBgColor;

    QFont propFont;
    QFont fixedFont;

    bool askForGameFile;
    QString lastFileOpenDir;

    QStringList recentGamesList;
    static const int recentGamesCapacity = 10;

    QSize appSize;
};


#endif
