#include "videoplayergst_p.h"
#include "videoplayer.h"

#include <QErrorMessage>
#include <QGst/Message>
#include <QGlib/Error>
#include <QGst/Pad>
#include <gst/video/video.h>

#include "rwopsappsrc.h"
#include "hmainwindow.h"

#ifdef emit
#undef emit
#endif
#include <QGlib/Signal>


void
VideoPlayer_priv::fSetupSource(QGst::ElementPtr source)
{
    if (not fAppSrc) {
        fAppSrc = new RwopsApplicationSource(q->fRwops);
    } else {
        fAppSrc->setSource(q->fRwops);
    }
    fAppSrc->setElement(source);
    fAppSrc->setStreamType(QGst::AppStreamTypeRandomAccess);
    fAppSrc->setSize(q->fDataLen);
}


void
VideoPlayer_priv::fOnBusMessage(const QGst::MessagePtr& message)
{
    switch (message->type()) {
        case QGst::MessageStateChanged: {
            if (message->source() != fPipeline) {
                // The state change doesn't belong to our pipeline.
                break;
            }
            // If the video is now ready to play, the pipeline will be in the
            // paused state and will have negotiated the caps. Extract the video
            // resolution from the caps object so that we can resize our window
            // in case it's smaller than us (we don't allow the video to scale
            // to a larger size than its own resolution.)
            QGst::StateChangedMessagePtr sc = message.staticCast<QGst::StateChangedMessage>();
            if (sc->newState() == QGst::StatePaused) {
                QGst::PadPtr vidpad(QGlib::emit<QGst::PadPtr>(fPipeline, "get-video-pad", 0));
                if (vidpad.isNull()) {
                    break;
                }
                QGst::CapsPtr caps = vidpad->negotiatedCaps();
                if (caps.isNull()) {
                    break;
                }
                gint vidWidth, vidHeight;
                QGlib::Value val(gst_video_get_size(vidpad, &vidWidth, &vidHeight));
                if (not val.toBool()) {
                    break;
                }
                // Make sure that we won't end up being enlarged; we only allow our
                // window to shrink. The video must be scaled down in that case.
                QSize vidSize(vidWidth, vidHeight);
                if (vidSize.width() > this->maximumWidth() or vidSize.height() > this->maximumHeight()) {
                    vidSize.scale(this->maximumSize(), Qt::KeepAspectRatio);
                }
                this->q->setGeometry(q->x() + (this->maximumWidth() - vidSize.width()) / 2,
                                     q->y() + (this->maximumHeight() - vidSize.height()) / 2,
                                     vidSize.width(), vidSize.height());
                this->q->setMaximumSize(vidSize);
                this->setMaximumSize(vidSize);
                this->q->show();
            }
            break;
        }

        case QGst::MessageEos:
            if (q->fLooping) {
                fPipeline->seek(QGst::FormatTime, QGst::SeekFlagFlush, 0);
            } else {
                q->stop();
                Q_EMIT videoFinished();
            }
            break;

        case QGst::MessageError: {
            QString msg(tr("Unable to play video: "));
            msg += message.staticCast<QGst::ErrorMessage>()->error().message();
            hMainWin->errorMsgObj()->showMessage(msg);
            q->stop();
            Q_EMIT errorOccurred();
            break;
        }

        default:
            break;
    }
}


void
VideoPlayer_priv::fOnVideoSinkChange(const QGst::MessagePtr& msg)
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
