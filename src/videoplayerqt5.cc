/* Copyright 2015 Nikos Chantziaras
 *
 * This file is part of Hugor.
 *
 * Hugor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Hugor is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Hugor.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with the Hugo Engine (or a modified version of the Hugo Engine), containing
 * parts covered by the terms of the Hugo License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 * Corresponding Source for a non-source form of such a combination shall
 * include the source code for the parts of the Hugo Engine used as well as
 * that of the covered work.
 */
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


void initVideoEngine(int&, char*[])
{ }

void closeVideoEngine()
{ }


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
    connect(d->fMediaPlayer, SIGNAL(error(QMediaPlayer::Error)),
            d, SLOT(onError(QMediaPlayer::Error)));
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
