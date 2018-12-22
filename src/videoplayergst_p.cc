// This is copyrighted software. More information is at the end of this file.
#include "videoplayergst_p.h"

#include <QAbstractEventDispatcher>
#include <QDebug>
#include <QErrorMessage>
#include <QThread>
#include <SDL_rwops.h>
#include <cstring>
#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/gstversion.h>
#include <gst/video/video.h>

#include "happlication.h"
#include "hmainwindow.h"
#include "videoplayer.h"

GMainLoop* VideoPlayer_priv::main_loop = nullptr;
GThread* VideoPlayer_priv::main_loop_thread = nullptr;

extern "C" {

static gpointer glibMainLoopThreadFunc(gpointer /*unused*/)
{
    VideoPlayer_priv::main_loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(VideoPlayer_priv::main_loop);
    g_main_loop_unref(VideoPlayer_priv::main_loop);
    VideoPlayer_priv::main_loop = nullptr;
    return nullptr;
}

} // extern "C"

VideoPlayer_priv::VideoPlayer_priv(QWidget* parent, VideoPlayer* qPtr)
    : QWidget(parent)
    , q(qPtr)
{
    memset(&appsrc_cbs, 0, sizeof(appsrc_cbs));

    // If the version of Qt we're running in does not use GLib, we need to start a GMainLoop so that
    // gstreamer can dispatch events.
    const QMetaObject* mo = QAbstractEventDispatcher::instance(hApp->thread())->metaObject();
    if (main_loop == nullptr && strcmp(mo->className(), "QEventDispatcherGlib") != 0
        && strcmp(mo->superClass()->className(), "QEventDispatcherGlib") != 0) {
        main_loop_thread = g_thread_new(nullptr, glibMainLoopThreadFunc, nullptr);
    }
}

extern "C" {

static void cbAppsrcNeedData(GstAppSrc* src, guint length, gpointer userData)
{
    if (length == (guint)-1) {
        // We're free to push any amount of bytes.
        length = 32768;
    }
    auto* rwops = static_cast<SDL_RWops*>(userData);
    GstBuffer* buffer;
    void* data;

#if GST_CHECK_VERSION(1, 0, 0)
    buffer = gst_buffer_new_allocate(nullptr, length, nullptr);
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
#if GST_CHECK_VERSION(1, 0, 0)
    gst_buffer_unmap(buffer, &mapInf);
#endif
    gst_app_src_push_buffer(src, buffer);
    // Indicate EOS if there's no more data to be had from the RWops.
    if (cnt < length) {
        gst_app_src_end_of_stream(src);
    }
}

static gboolean cbAppSrcSeekData(GstAppSrc* /*appsrc*/, guint64 offset, gpointer rwops)
{
    return SDL_RWseek(static_cast<SDL_RWops*>(rwops), offset, RW_SEEK_SET) == (int)offset;
}

} // extern "C"

void VideoPlayer_priv::adjustForVideoSize(QSize vidSize)
{
    if (vidSize.width() > maximumWidth() or vidSize.height() > maximumHeight()) {
        vidSize.scale(maximumSize(), Qt::KeepAspectRatio);
    }
    q->setGeometry(q->x() + (maximumWidth() - vidSize.width()) / 2,
                   q->y() + (maximumHeight() - vidSize.height()) / 2, vidSize.width(),
                   vidSize.height());
    q->setMaximumSize(vidSize);
    setMaximumSize(vidSize);
}

void VideoPlayer_priv::cbOnSourceSetup(GstAppSrc* source, VideoPlayer_priv* d)
{
    d->appsrc = source;
    gst_app_src_set_stream_type(source, GST_APP_STREAM_TYPE_RANDOM_ACCESS);
    g_object_set(G_OBJECT(source), "format", GST_FORMAT_BYTES, NULL);
    memset(&d->appsrc_cbs, 0, sizeof(d->appsrc_cbs));
    d->appsrc_cbs.need_data = cbAppsrcNeedData;
    d->appsrc_cbs.enough_data = nullptr;
    d->appsrc_cbs.seek_data = cbAppSrcSeekData;
    gst_app_src_set_callbacks(source, &d->appsrc_cbs, d->q->rwops_, nullptr);
    gst_app_src_set_size(source, d->q->data_len);
}

void VideoPlayer_priv::cbOnBusMessage(GstMessage* message, VideoPlayer_priv* d)
{
    Qt::ConnectionType conType =
        main_loop == nullptr ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_STATE_CHANGED: {
        if (GST_MESSAGE_SRC(message) != GST_OBJECT(d->pipeline)) {
            // The state change doesn't belong to our pipeline.
            break;
        }
        // If the video is now ready to play, the pipeline will be in the paused state and will have
        // negotiated the caps. Extract the video resolution from the caps object so that we can
        // resize our window in case it's smaller than us (we don't allow the video to scale to a
        // larger size than its own resolution.)
        GstState oldState, newState, pendingState;
        gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
        if (newState == GST_STATE_PLAYING) {
            QMetaObject::invokeMethod(d->q, "show", conType);
        }
        if (newState == GST_STATE_PAUSED) {
            GstPad* vidpad = nullptr;
            g_signal_emit_by_name(d->pipeline, "get-video-pad", 0, &vidpad, 0);
            if (vidpad == nullptr) {
                break;
            }
#if GST_CHECK_VERSION(1, 0, 0)
            GstCaps* caps = gst_pad_get_current_caps(vidpad);
#else
            GstCaps* caps = gst_pad_get_negotiated_caps(vidpad);
#endif
            if (caps == nullptr) {
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
            // Make sure that we won't end up being enlarged; we only allow our window to shrink.
            // The video must be scaled down if it's too large.
            QMetaObject::invokeMethod(d, "adjustForVideoSize", conType,
                                      Q_ARG(QSize, QSize(vidWidth, vidHeight)));
        }
        break;
    }

    case GST_MESSAGE_SEGMENT_DONE:
        if (not gst_element_seek_simple(d->pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_SEGMENT, 0)) {
            qWarning() << "Sending video seek event failed.";
        }
        break;

    case GST_MESSAGE_EOS:
        QMetaObject::invokeMethod(d->q, "stop", conType);
        break;

    case GST_MESSAGE_ERROR: {
        QString errorStr(tr("Unable to play video: "));
        GError* gErr = nullptr;
        gst_message_parse_error(message, &gErr, nullptr);
        errorStr += gErr->message;
        QMetaObject::invokeMethod(hMainWin->errorMsgObj(), "showMessage", Q_ARG(QString, errorStr));
        QMetaObject::invokeMethod(d->q, "stop", conType);
        g_error_free(gErr);
        emit d->q->errorOccurred();
        break;
    }

    default:
        break;
    }
}

/* Copyright (C) 2011-2018 Nikos Chantziaras
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
