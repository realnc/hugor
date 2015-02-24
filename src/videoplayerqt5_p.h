#ifndef VIDEOPLAYERQT5_P
#define VIDEOPLAYERQT5_P

#include <QWidget>
#include <QMediaPlayer>


class VideoPlayer_priv: public QWidget {
    Q_OBJECT

public:
    VideoPlayer_priv(QWidget* parent, class VideoPlayer* qPtr)
        : QWidget(parent),
          q(qPtr),
          fMediaPlayer(0),
          fVideoWidget(0)
    { }

    class VideoPlayer* q;
    class QMediaPlayer* fMediaPlayer;
    class QVideoWidget* fVideoWidget;
    class RwopsQIODevice* fIODev;

public slots:
    void onStatusChange(QMediaPlayer::MediaStatus status);
    void onError(QMediaPlayer::Error error);
};


#endif
