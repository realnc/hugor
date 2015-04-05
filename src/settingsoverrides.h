#ifndef SETTINGSOVERRIDES_H
#define SETTINGSOVERRIDES_H

#include <QString>
#include <QColor>


class SettingsOverrides {
  public:
    SettingsOverrides( const QString& filename );

    QString appName;
    QString authorName;
    bool fullscreen;
    int fullscreenWidth;
    bool hideMenuBar;
    int marginSize;
    QColor fsMarginColor;
    int widthRatio;
    int heightRatio;
    int propFontSize;
    int fixedFontSize;
    int scrollbackFontSize;
    bool imageSmoothing;
    bool muteWhenMinimized;
};


#endif // SETTINGSOVERRIDES_H
