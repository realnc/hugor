#include "videoplayer.h"
#include "videoplayerqt5_p.h"

#include <QErrorMessage>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <SDL_rwops.h>

#include <QFile>

#include "rwopsbundle.h"
#include "rwopsqiodev.h"
#include "hmainwindow.h"


VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent),
      fRwops(0)
{
    d = new VideoPlayer_priv(this, this);
    d->fMediaPlayer = new QMediaPlayer(this, QMediaPlayer::StreamPlayback);
    //d->fMediaPlayer = new QMediaPlayer(this);
    d->fVideoWidget = new QVideoWidget(this);

    //d->fVideoWidget->setAttribute(Qt::WA_OpaquePaintEvent, true);
//    d->fVideoWidget->setAttribute(Qt::WA_NoSystemBackground, true);
//    d->fVideoWidget->setAttribute(Qt::WA_PaintOnScreen, true);
//    d->fVideoWidget->setAutoFillBackground(false);
    d->fVideoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    d->fMediaPlayer->setVideoOutput(d->fVideoWidget);
    d->fIODev = new RwopsQIODevice(this);
    // So that the mouse cursor can be made visible again when moving the mouse.
    this->setMouseTracking(true);
    d->fVideoWidget->setMouseTracking(true);

    connect(d->fMediaPlayer, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            d, SLOT(onStatusChange(QMediaPlayer::MediaStatus)));
    connect(d, SIGNAL(videoFinished()), SIGNAL(videoFinished()));
    connect(d->fMediaPlayer, SIGNAL(error(QMediaPlayer::Error)),
            d, SLOT(onError(QMediaPlayer::Error)));
    connect(d, SIGNAL(errorOccurred()), SIGNAL(errorOccurred()));
}


VideoPlayer::~VideoPlayer()
{
    if (fRwops) {
        SDL_RWclose(fRwops);
    }
}


bool
VideoPlayer::loadVideo(FILE* src, long len, bool loop)
{
    this->stop();
    if (fRwops) {
        d->fIODev->close();
        SDL_RWclose(fRwops);
    }
    fRwops = RWFromMediaBundle(src, len);
    if (not fRwops) {
        hMainWin->errorMsgObj()->showMessage(tr("Unable to read video data from disk: ")
                                             + SDL_GetError());
        return false;
    }
    d->fIODev->open(fRwops, QIODevice::ReadOnly);
    d->fMediaPlayer->setMedia(QUrl(), d->fIODev);
    fDataLen = len;
    fLooping = loop;
    return true;
}


void
VideoPlayer::play()
{
    d->fVideoWidget->show();
    d->fVideoWidget->raise();
    d->fMediaPlayer->play();
}


void
VideoPlayer::stop()
{
    d->fMediaPlayer->stop();
}


void
VideoPlayer::setVolume(int vol)
{
    d->fMediaPlayer->setVolume(vol);
}


void
VideoPlayer::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    d->fVideoWidget->resize(this->size());
}
