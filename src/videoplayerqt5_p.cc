/* Copyright 2015 Nikos Chantziaras
 *
 * This file is part of Hugor.
 *
 * Hugor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Foobar is distributed in the hope that it will be useful, but WITHOUT ANY
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
 * parts covered by the terms of the Hugo Engine License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 * Corresponding Source for a non-source form of such a combination shall
 * include the source code for the parts of the Hugo Engine used as well as
 * that of the covered work.
 */
#include "videoplayer.h"
#include "videoplayerqt5_p.h"

#include <QErrorMessage>

#include "hmainwindow.h"


void
VideoPlayer_priv::onStatusChange(QMediaPlayer::MediaStatus status)
{
    if (status != QMediaPlayer::EndOfMedia) {
        return;
    }
    if (q->fLooping) {
        q->play();
    } else {
        emit q->videoFinished();
    }
}


void
VideoPlayer_priv::onError(QMediaPlayer::Error error)
{
    if (error == QMediaPlayer::NoError) {
        return;
    }
    QString msg(tr("An error occurred while trying to play video"));
    if (not fMediaPlayer->errorString().isEmpty()) {
        msg += ": ";
        msg += fMediaPlayer->errorString();
    } else {
        msg += ", but I don't know what went wrong.";
    }
    hMainWin->errorMsgObj()->showMessage(msg);
    emit q->errorOccurred();
}
