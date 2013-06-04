#include "gstvideoplayer.h"

#include <QDir>
#include <QUrl>
#include <QErrorMessage>
#include <QGlib/Connect>
#include <QGlib/Error>
#include <QGst/Pipeline>
#include <QGst/ElementFactory>
#include <QGst/Bus>
#include <QGst/Message>
#include <QGst/Query>
#include <QGst/ClockTime>
#include <QGst/Event>
#include <QGst/StreamVolume>
#include <SDL_rwops.h>

#include "hmainwindow.h"
#include "rwopsbundle.h"
#include "rwopsappsrc.h"


VideoPlayer::VideoPlayer(QWidget *parent)
    : QGst::Ui::VideoWidget(parent),
      fRwops(0),
      fAppSrc(0)
{
    // So that the mouse cursor can be made visible again when moving the mouse.
    this->setMouseTracking(true);
}


VideoPlayer::~VideoPlayer()
{
    if (fPipeline) {
        fPipeline->setState(QGst::StateNull);
        fPipeline->bus()->disableSyncMessageEmission();
        stopPipelineWatch();
    }
    if (fRwops) {
        SDL_RWclose(fRwops);
    }
}


bool
VideoPlayer::loadVideo(FILE* src, long len, bool loop)
{
    if (not fPipeline) {
        fPipeline = QGst::ElementFactory::make("playbin2").dynamicCast<QGst::Pipeline>();
        if (not fPipeline) {
            hMainWin->errorMsgObj()->showMessage(tr("Unable to play video. You are "
                                                    "probably missing the GStreamer plugins "
                                                    "from the \"gst-plugins-base\" set."));
            return false;
        }
        // Let the video widget watch the pipeline for new video sinks. This
        // makes sure that our superclass will set up the video sink correctly.
        this->watchPipeline(fPipeline);
        QGst::BusPtr bus = fPipeline->bus();
        // We need to be informed when the playback state changes.
        bus->addSignalWatch();
        QGlib::connect(bus, "message", this, &VideoPlayer::fOnBusMessage);
        // We need to be informed when a video sink is added.
        bus->enableSyncMessageEmission();
        QGlib::connect(bus, "sync-message", this, &VideoPlayer::fOnVideoSinkChange);
        QGlib::connect(fPipeline, "source-setup", this, &VideoPlayer::fSetupSource);
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
    fPipeline->setProperty("uri", "appsrc://");
    return true;
}


void
VideoPlayer::fSetupSource(QGst::ElementPtr source)
{
    if (not fAppSrc) {
        fAppSrc = new RwopsApplicationSource(fRwops);
    } else {
        fAppSrc->setSource(fRwops);
    }
    fAppSrc->setElement(source);
    fAppSrc->setStreamType(QGst::AppStreamTypeRandomAccess);
    fAppSrc->setSize(fDataLen);
}


void
VideoPlayer::play()
{
    if (fPipeline) {
        fPipeline->setState(QGst::StatePlaying);
    }
}


void
VideoPlayer::stop()
{
    if (fPipeline) {
        fPipeline->setState(QGst::StateNull);
    }
}


void
VideoPlayer::setVolume(int vol)
{
    if (not fPipeline) {
        return;
    }
    QGst::StreamVolumePtr svp = fPipeline.dynamicCast<QGst::StreamVolume>();
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
VideoPlayer::fOnBusMessage(const QGst::MessagePtr & message)
{
    switch (message->type()) {
        case QGst::MessageEos:
            if (fLooping) {
                fPipeline->seek(QGst::FormatTime, QGst::SeekFlagFlush, 0);
            } else {
                this->stop();
                emit videoFinished();
            }
            break;

        case QGst::MessageError: {
            QString msg(tr("Unable to play video: "));
            msg += message.staticCast<QGst::ErrorMessage>()->error().message();
            hMainWin->errorMsgObj()->showMessage(msg);
            this->stop();
            emit errorOccured();
            break;
        }

        default:
            break;
    }
}


void
VideoPlayer::fOnVideoSinkChange(const QGst::MessagePtr& msg)
{
    if (msg->type() == QGst::MessageElement
        and msg->internalStructure()->name() == QLatin1String("prepare-xwindow-id"))
    {
        this->videoSink()->setProperty("force-aspect-ratio", true);
        QGst::ElementPtr sink = fPipeline->property("video-sink").get<QGst::ElementPtr>();
        if (sink) {
            sink->setProperty("force-aspect-ratio", true);
        }
    }
}
