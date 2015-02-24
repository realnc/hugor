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
