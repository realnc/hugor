// This is copyrighted software. More information is at the end of this file.
#include "hugodefs.h"
#include "videoplayer.h"

#include <QErrorMessage>
#include <QFile>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <SDL_rwops.h>

#include "hmainwindow.h"
#include "hugorfile.h"
#include "rwopsbundle.h"
#include "rwopsqiodev.h"
#include "videoplayerqt5_p.h"

void initVideoEngine(int& /*argc*/, char* /*argv*/[])
{}

void closeVideoEngine()
{}

void muteVideo(bool /*mute*/)
{
    // TODO
}

void updateVideoVolume()
{
    // TODO
}

VideoPlayer::VideoPlayer(QWidget* parent)
    : QWidget(parent)
    , d_(new VideoPlayer_priv(this, this))
{
    d_->media_player = new QMediaPlayer(this, QMediaPlayer::LowLatency);
    d_->setAttribute(Qt::WA_OpaquePaintEvent, true);
    // d_->setAttribute(Qt::WA_NoSystemBackground, true);
    // d_->setAttribute(Qt::WA_PaintOnScreen, true);
    // d_->setAutoFillBackground(false);
    d_->setAspectRatioMode(Qt::KeepAspectRatio);
    d_->media_player->setVideoOutput(d_);
    d_->io_dev = new RwopsQIODevice(this);
    // So that the mouse cursor can be made visible again when moving the mouse.
    setMouseTracking(true);
    d_->setMouseTracking(true);

    connect(d_->media_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), d_,
            SLOT(onStatusChange(QMediaPlayer::MediaStatus)));
    connect(d_->media_player, SIGNAL(error(QMediaPlayer::Error)), d_,
            SLOT(onError(QMediaPlayer::Error)));
}

VideoPlayer::~VideoPlayer()
{
    if (rwops_ != nullptr) {
        SDL_RWclose(rwops_);
    }
}

bool VideoPlayer::loadVideo(HugorFile* src, long len, bool loop)
{
    stop();
    if (rwops_ != nullptr) {
        d_->io_dev->close();
        SDL_RWclose(rwops_);
    }
    rwops_ = RWFromMediaBundle(src->get(), len);
    if (rwops_ == nullptr) {
        hMainWin->errorMsgObj()->showMessage(tr("Unable to read video data from disk: ")
                                             + SDL_GetError());
        return false;
    }
    src->release();
    d_->io_dev->open(rwops_, QIODevice::ReadOnly);
    d_->media_player->setMedia(QUrl(), d_->io_dev);
    data_len = len;
    is_looping = loop;
    return true;
}

void VideoPlayer::play()
{
    show();
    d_->show();
    // raise();
    // d_->raise();
    d_->media_player->play();
}

void VideoPlayer::stop()
{
    d_->media_player->stop();
    hide();
}

void VideoPlayer::updateVolume()
{
    // TODO
}

void VideoPlayer::setVolume(int vol)
{
    d_->media_player->setVolume(vol);
}

void VideoPlayer::setMute(bool /*mute*/)
{
    // TODO
}

void VideoPlayer::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    d_->resize(size());
}

QPaintEngine* VideoPlayer::paintEngine() const
{
    return QWidget::paintEngine();
}

/* Copyright (C) 2011-2019 Nikos Chantziaras
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
