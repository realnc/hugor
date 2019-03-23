// This is copyrighted software. More information is at the end of this file.
#include "hugodefs.h"
#include "videoplayer.h"

#include <QDebug>
#include <QErrorMessage>
#include <QMessageBox>
#include <QResizeEvent>
#include <SDL_rwops.h>
#include <cmath>
#include <glib.h>
#include <gst/gstelement.h>
#include <gst/gstpipeline.h>
#include <gst/gstversion.h>
#include <gst/video/video.h>
#if GST_CHECK_VERSION(1, 0, 0)
#include <gst/video/videooverlay.h>
#else
#include <gst/interfaces/xoverlay.h>
#endif

#include "happlication.h"
#include "hframe.h"
#include "hmainwindow.h"
#include "hugorfile.h"
#include "rwopsbundle.h"
#include "settings.h"
#include "videoplayergst_p.h"

static bool isMuted = false;
static VideoPlayer* currentVideo = nullptr;

void initVideoEngine(int& argc, char* argv[])
{
    GError* gstError = nullptr;

    if (not gst_init_check(&argc, &argv, &gstError)) {
        QString errMsg(
            QObject::tr("Unable to use GStreamer. Video support will be "
                        "disabled."));
        if (gstError->message != nullptr && qstrlen(gstError->message) > 0) {
            errMsg += QObject::tr("The GStreamer error was: ")
                      + QString::fromLocal8Bit(gstError->message);
        }
        g_error_free(gstError);
        QMessageBox::critical(nullptr, HApplication::applicationName(), errMsg);
        hApp->settings()->video_sys_error = true;
    }
}

void closeVideoEngine()
{}

void muteVideo(bool mute)
{
    if (mute and not isMuted) {
        isMuted = true;
        if (currentVideo != nullptr) {
            currentVideo->setMute(true);
        }
    } else if (not mute and isMuted) {
        isMuted = false;
        if (currentVideo != nullptr) {
            currentVideo->setMute(false);
        }
    }
}

void updateVideoVolume()
{
    if (currentVideo != nullptr) {
        currentVideo->updateVolume();
    }
}

VideoPlayer::VideoPlayer(QWidget* parent)
    : QWidget(parent)
    , d_(new VideoPlayer_priv(this, this))
{
    d_->winId(); // Enforce a native window handle for gstreamer.
    d_->setUpdatesEnabled(false); // Don't fight with gstreamer over updates.
    // So that the mouse cursor can be made visible again when moving the mouse.
    setMouseTracking(true);
    d_->setMouseTracking(true);
}

VideoPlayer::~VideoPlayer()
{
    if (d_->pipeline != nullptr) {
        ::currentVideo = nullptr;
        gst_bus_remove_signal_watch(d_->bus);
#if not GST_CHECK_VERSION(1, 0, 0)
        gst_bus_disable_sync_message_emission(d_->bus);
#endif
        gst_element_set_state(d_->pipeline, GST_STATE_NULL);
        gst_object_unref(d_->bus);
        gst_object_unref(d_->pipeline);
    }
    if (rwops_ != nullptr) {
        SDL_RWclose(rwops_);
    }
}

extern "C" {

#if not GST_CHECK_VERSION(1, 0, 0)
static void cbSyncMessage(GstBus*, GstMessage* message, gpointer userData)
{
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ELEMENT
        and gst_structure_has_name(message->structure, "prepare-xwindow-id")) {
        GstElement* sink = GST_ELEMENT(GST_MESSAGE_SRC(message));
        gst_x_overlay_set_window_handle(GST_X_OVERLAY(GST_MESSAGE_SRC(message)),
                                        (guintptr) static_cast<QWidget*>(userData)->winId());
        g_object_set(sink, "force-aspect-ratio", true, NULL);
    }
}
#endif

static void cbOnBusMessage(GstBus* /*bus*/, GstMessage* message, gpointer d)
{
    VideoPlayer_priv::cbOnBusMessage(message, static_cast<VideoPlayer_priv*>(d));
}

static void cbOnSourceSetup(GstPipeline* /*pipeline*/, GstAppSrc* source, gpointer d)
{
    VideoPlayer_priv::cbOnSourceSetup(source, static_cast<VideoPlayer_priv*>(d));
}

} // extern "C"

bool VideoPlayer::loadVideo(HugorFile* src, long len, bool loop)
{
    if (d_->pipeline == nullptr) {
        const char* playbinName =
#if GST_CHECK_VERSION(1, 0, 0)
            "playbin";
#else
            "playbin2";
#endif
        d_->pipeline = gst_element_factory_make(playbinName, nullptr);
        if (d_->pipeline == nullptr) {
            hMainWin->errorMsgObj()->showMessage(
                tr("Unable to play video. You are "
                   "probably missing the GStreamer plugins "
                   "from the \"gst-plugins-base\" set."));
            return false;
        }

        d_->bus = gst_pipeline_get_bus(GST_PIPELINE(d_->pipeline));
        // We need to be informed when the playback state changes.
        gst_bus_add_signal_watch(d_->bus);
        g_signal_connect(d_->bus, "message", G_CALLBACK(cbOnBusMessage), d_);
#if GST_CHECK_VERSION(1, 0, 0)
        // With gst 1.x, we can configure aspect ratio and set the window ID
        // directly on playbin.
        g_object_set(d_->pipeline, "force-aspect-ratio", true, NULL);
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(d_->pipeline), (guintptr)d_->winId());
#else
        // With gst 0.10, we need to be informed when a video sink is added so we can configure it
        // later.
        gst_bus_enable_sync_message_emission(d_->bus);
        g_signal_connect(d_->bus, "sync-message", G_CALLBACK(cbSyncMessage), d_);
#endif
        g_signal_connect(d_->pipeline, "source-setup", G_CALLBACK(cbOnSourceSetup), d_);
    }
    if (rwops_ != nullptr) {
        SDL_RWclose(rwops_);
    }
    rwops_ = RWFromMediaBundle(src->get(), len);
    if (rwops_ == nullptr) {
        hMainWin->errorMsgObj()->showMessage(tr("Unable to read video data from disk: ")
                                             + SDL_GetError());
        return false;
    }
    src->release();
    data_len = len;
    is_looping = loop;
    g_object_set(G_OBJECT(d_->pipeline), "uri", "appsrc://", NULL);
    return true;
}

void VideoPlayer::play()
{
    if (d_->pipeline == nullptr) {
        return;
    }

    hFrame->updateGameScreen(true);

    d_->setMaximumSize(maximumSize());
    ::currentVideo = this;
    updateVolume();
    gst_element_set_state(d_->pipeline, GST_STATE_PLAYING);
    hApp->advanceEventLoop();
    if (is_looping) {
        // Wait for the pipeline to transition into the playing state.
        gst_element_get_state(d_->pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
        // Do an initial seek so that there's no pause before the first segment message arrives.
        if (not gst_element_seek(
                d_->pipeline, 1.0, GST_FORMAT_TIME,
                (GstSeekFlags)((int)GST_SEEK_FLAG_FLUSH | (int)GST_SEEK_FLAG_SEGMENT),
                GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
            qWarning() << "Sending initial video seek event failed.";
            stop();
        }
    }
}

void VideoPlayer::stop()
{
    if (d_->pipeline != nullptr) {
        gst_element_set_state(d_->pipeline, GST_STATE_NULL);
        emit videoFinished();
        hide();
    }
}

void VideoPlayer::updateVolume()
{
    if (d_->pipeline != nullptr) {
        setVolume(d_->volume);
    }
}

void VideoPlayer::setVolume(int vol)
{
    if (d_->pipeline == nullptr) {
        return;
    }
    if (vol < 0) {
        vol = 0;
    } else if (vol > 100) {
        vol = 100;
    }
    d_->volume = vol;

    // Attenuate the result by the global volume setting. Use an exponential volume scale. Use the
    // second power instead of the third to be consistent with the SDL audio volume.
    g_object_set(d_->pipeline, "volume",
                 std::pow(((gdouble)vol * hApp->settings()->sound_volume) / 10000.0, 2), NULL);
}

void VideoPlayer::setMute(bool mute)
{
    if (d_->pipeline != nullptr) {
        g_object_set(d_->pipeline, "mute", mute, NULL);
    }
}

void VideoPlayer::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    d_->resize(e->size());
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
