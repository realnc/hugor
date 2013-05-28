#ifndef SETTINGSOVERRIDES_H
#define SETTINGSOVERRIDES_H

#include <QString>


class SettingsOverrides {
  private:
    class QSettings* fSett;

  public:
    SettingsOverrides( const QString& filename );

    QString appName;
    QString authorName;
    bool fullscreen;
    bool hideMenuBar;
    int fullscreenWidth;
};


#endif // SETTINGSOVERRIDES_H
