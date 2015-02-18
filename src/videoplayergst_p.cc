#include "videoplayergst_p.h"
#include "videoplayer.h"

#include <QErrorMessage>
#include <QDebug>
#include <SDL_rwops.h>
#include <glib.h>
#include <gst/gstversion.h>
#include <gst/video/video.h>
#include <gst/app/gstappsrc.h>

#include "hmainwindow.h"


extern "C" {

static void
cbAppsrcNeedData(GstAppSrc* src, guint length, gpointer userData)
{
    SDL_RWops* rwops = static_cast<SDL_RWops*>(userData);
    GstBuffer* buffer;
    void* data;

#if GST_CHECK_VERSION(1, 0, 0)
    buffer = gst_buffer_new_allocate(NULL, length, NULL);
    GstMapInfo mapInf;
    if (not gst_buffer_map(buffer, &mapInf, GST_MAP_WRITE)) {
        qWarning() << "Can't map GstBuffer memory.";
        gst_app_src_end_of_stream(src);
        return;
    }
    data = mapInf.data;
#else
    buffer = gst_buffer_new_and_alloc(length);
    data = buffer->data;
#endif

    long cnt = SDL_RWread(rwops, data, 1, length);
    gst_app_src_push_buffer(src, buffer);
    // Indicate EOS if there's no more data to be had from the RWops.
    if (cnt < length) {
        gst_app_src_end_of_stream(src);
    }
}

static gboolean
cbAppSrcSeekData(GstAppSrc*, guint64 offset, gpointer rwops)
{
    return SDL_RWseek(static_cast<SDL_RWops*>(rwops), offset, RW_SEEK_SET) == (int)offset;
}

} // extern "C"


void
VideoPlayer_priv::cbOnSourceSetup(GstPipeline*, GstAppSrc* source, VideoPlayer_priv* d)
{
    d->fAppSrc = source;
    gst_app_src_set_stream_type(source, GST_APP_STREAM_TYPE_RANDOM_ACCESS);
    g_object_set(G_OBJECT(source), "format", GST_FORMAT_BYTES, NULL);
    memset(&d->fAppSrcCbs, 0, sizeof(d->fAppSrcCbs));
    d->fAppSrcCbs.need_data = cbAppsrcNeedData;
    d->fAppSrcCbs.enough_data = 0;
    d->fAppSrcCbs.seek_data = cbAppSrcSeekData;
    gst_app_src_set_callbacks(source, &d->fAppSrcCbs, d->q->fRwops, 0);
    gst_app_src_set_size(source, d->q->fDataLen);
}


void
VideoPlayer_priv::cbOnBusMessage(GstBus*, GstMessage* message, VideoPlayer_priv* d)
{
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_STATE_CHANGED: {
            if (GST_MESSAGE_SRC(message) != GST_OBJECT(d->fPipeline)) {
                // The state change doesn't belong to our pipeline.
                break;
            }
            // If the video is now ready to play, the pipeline will be in the
            // paused state and will have negotiated the caps. Extract the video
            // resolution from the caps object so that we can resize our window
            // in case it's smaller than us (we don't allow the video to scale
            // to a larger size than its own resolution.)
            GstState oldState, newState, pendingState;
            gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
            if (newState == GST_STATE_PAUSED) {
                GstPad* vidpad = 0;
                g_signal_emit_by_name(d->fPipeline, "get-video-pad", 0, &vidpad, 0);
                if (vidpad == 0) {
                    break;
                }
#if GST_CHECK_VERSION(1, 0, 0)
                GstCaps* caps = gst_pad_get_current_caps(vidpad);
#else
                GstCaps* caps = gst_pad_get_negotiated_caps(vidpad);
#endif
                if (caps == 0) {
                    break;
                }
                gst_caps_unref(caps);
                gint vidWidth, vidHeight;
#if GST_CHECK_VERSION(1, 0, 0)
                GstVideoInfo vidInfo;
                gst_video_info_init(&vidInfo);
                if (gst_video_info_from_caps(&vidInfo, caps)) {
                    vidWidth = vidInfo.width;
                    vidHeight = vidInfo.height;
                } else {
                    break;
                }
#else
                if (not gst_video_get_size(vidpad, &vidWidth, &vidHeight)) {
                    break;
                }
#endif
                // Make sure that we won't end up being enlarged; we only allow our
                // window to shrink. The video must be scaled down in that case.
                QSize vidSize(vidWidth, vidHeight);
                if (vidSize.width() > d->maximumWidth() or vidSize.height() > d->maximumHeight()) {
                    vidSize.scale(d->maximumSize(), Qt::KeepAspectRatio);
                }
                d->q->setGeometry(d->q->x() + (d->maximumWidth() - vidSize.width()) / 2,
                                  d->q->y() + (d->maximumHeight() - vidSize.height()) / 2,
                                  vidSize.width(), vidSize.height());
                d->q->setMaximumSize(vidSize);
                d->setMaximumSize(vidSize);
                d->q->show();
            }
            break;
        }

        case GST_MESSAGE_SEGMENT_DONE:
            if (not gst_element_seek_simple(d->fPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_SEGMENT, 0)) {
                qWarning() << "Sending video seek event failed.";
            }
            break;

        case GST_MESSAGE_EOS:
            d->q->stop();
            emit d->videoFinished();
            break;

        case GST_MESSAGE_ERROR: {
            QString errorStr(tr("Unable to play video: "));
            GError* gErr = 0;
            gst_message_parse_error(message, &gErr, 0);
            errorStr += gErr->message;
            hMainWin->errorMsgObj()->showMessage(errorStr);
            d->q->stop();
            g_error_free(gErr);
            emit d->errorOccurred();
            break;
        }

        default:
            break;
    }
    // TODO: is this needed?
    //gst_object_unref(bus);
    //gst_message_unref(message);
}
