#include "videoplayer.h"
#include "videoplayergst_p.h"

#include <QResizeEvent>
#include <QErrorMessage>
#include <QGlib/Connect>
#include <QGst/ElementFactory>
#include <QGst/Bus>
#include <QGst/Message>
#include <QGst/StreamVolume>
#include <SDL_rwops.h>

#include "hmainwindow.h"
#include "rwopsbundle.h"
#include "rwopsappsrc.h"


VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent),
      fRwops(0)
{
    this->d = new VideoPlayer_priv(this, this);
    connect(d, SIGNAL(videoFinished()), SIGNAL(videoFinished()));
    connect(d, SIGNAL(errorOccurred()), SIGNAL(errorOccurred()));
    // So that the mouse cursor can be made visible again when moving the mouse.
    this->setMouseTracking(true);
    d->setMouseTracking(true);
}


VideoPlayer::~VideoPlayer()
{
    if (d->fPipeline) {
        d->fPipeline->setState(QGst::StateNull);
        d->fPipeline->bus()->disableSyncMessageEmission();
        d->stopPipelineWatch();
    }
    if (fRwops) {
        SDL_RWclose(fRwops);
    }
}


bool
VideoPlayer::loadVideo(FILE* src, long len, bool loop)
{
    if (not d->fPipeline) {
        d->fPipeline = QGst::ElementFactory::make("playbin2").dynamicCast<QGst::Pipeline>();
        if (not d->fPipeline) {
            hMainWin->errorMsgObj()->showMessage(tr("Unable to play video. You are "
                                                    "probably missing the GStreamer plugins "
                                                    "from the \"gst-plugins-base\" set."));
            return false;
        }
        // Let the video widget watch the pipeline for new video sinks. This
        // makes sure that our superclass will set up the video sink correctly.
        d->watchPipeline(d->fPipeline);
        QGst::BusPtr bus = d->fPipeline->bus();
        // We need to be informed when the playback state changes.
        bus->addSignalWatch();
        QGlib::connect(bus, "message", d, &VideoPlayer_priv::fOnBusMessage);
        // We need to be informed when a video sink is added.
        bus->enableSyncMessageEmission();
        QGlib::connect(bus, "sync-message", d, &VideoPlayer_priv::fOnVideoSinkChange);
        QGlib::connect(d->fPipeline, "source-setup", d, &VideoPlayer_priv::fSetupSource);
    }
    if (fRwops) {
        SDL_RWclose(fRwops);
    }
    fRwops = RWFromMediaBundle(src, len);
    if (not fRwops) {
        hMainWin->errorMsgObj()->showMessage(tr("Unable to read video data from disk: ")
                                             + SDL_GetError());
        return false;
    }
    fDataLen = len;
    fLooping = loop;
    d->fPipeline->setProperty("uri", "appsrc://");
    return true;
}


void
VideoPlayer::play()
{
    if (d->fPipeline) {
        d->fPipeline->setState(QGst::StatePlaying);
    }
}


void
VideoPlayer::stop()
{
    if (d->fPipeline) {
        d->fPipeline->setState(QGst::StateNull);
        emit videoFinished();
    }
}


void
VideoPlayer::setVolume(int vol)
{
    if (not d->fPipeline) {
        return;
    }
    QGst::StreamVolumePtr svp = d->fPipeline.dynamicCast<QGst::StreamVolume>();
    if (svp) {
        if (vol < 0) {
            vol = 0;
        } else if (vol > 100) {
            vol = 100;
        }
        svp->setVolume((double)vol / 100.0, QGst::StreamVolumeFormatLinear);
    }
}


void
VideoPlayer::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    d->resize(e->size());
}
