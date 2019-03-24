// This is copyrighted software. More information is at the end of this file.
#include "videoplayerqt5_p.h"

#include <QErrorMessage>

#include "hmainwindow.h"
#include "videoplayer.h"

void VideoPlayer_priv::onStatusChange(QMediaPlayer::MediaStatus status)
{
    if (status != QMediaPlayer::EndOfMedia) {
        return;
    }
    if (q->is_looping) {
        media_player->play();
    } else {
        emit q->videoFinished();
    }
}

void VideoPlayer_priv::onError(QMediaPlayer::Error error)
{
    if (error == QMediaPlayer::NoError) {
        return;
    }
    QString msg(tr("An error occurred while trying to play video"));
    if (not media_player->errorString().isEmpty()) {
        msg += ": ";
        msg += media_player->errorString();
    } else {
        msg += ", but I don't know what went wrong.";
    }
    hMainWin->errorMsgObj()->showMessage(msg);
    emit q->errorOccurred();
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
 */
