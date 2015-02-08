#ifndef SETTINGSOVERRIDES_H
#define SETTINGSOVERRIDES_H

#include <QString>


class SettingsOverrides {
  public:
    SettingsOverrides( const QString& filename );

    QString appName;
    QString authorName;
    bool fullscreen;
    int fullscreenWidth;
    bool hideMenuBar;
    int marginSize;
    int widthRatio;
    int heightRatio;
    int propFontSize;
    int fixedFontSize;
    bool imageSmoothing;
    bool pauseAudio;
};


#endif // SETTINGSOVERRIDES_H
