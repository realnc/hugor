/* Copyright 2015 Nikos Chantziaras
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
#include "videoplayer.h"
#include "videoplayergst_p.h"

#include <QResizeEvent>
#include <QErrorMessage>
#include <QDebug>
#include <cmath>
#include <glib.h>
#include <gst/gstversion.h>
#include <gst/gstelement.h>
#include <gst/gstpipeline.h>
#if GST_CHECK_VERSION(1, 0, 0)
#include <gst/video/videooverlay.h>
#else
#include <gst/interfaces/xoverlay.h>
#endif
#include <gst/video/video.h>
#include <SDL_rwops.h>

#include "happlication.h"
#include "hmainwindow.h"
#include "hframe.h"
#include "settings.h"
#include "rwopsbundle.h"
#include "hugodefs.h"


static bool isMuted = false;
static VideoPlayer* currentVideo = 0;


void muteVideo(bool mute)
{
    if (mute and not isMuted) {
        isMuted = true;
        if (currentVideo) {
            currentVideo->setMute(true);
        }
    } else if (not mute and isMuted) {
        isMuted = false;
        if (currentVideo) {
            currentVideo->setMute(false);
        }
    }
}


void updateVideoVolume()
{
    if (currentVideo) {
        currentVideo->updateVolume();
    }
}


VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent),
      fRwops(0)
{
    this->d = new VideoPlayer_priv(this, this);
    d->winId(); // Enforce a native window handle for gstreamer.
    d->setUpdatesEnabled(false); // Don't fight with gstreamer over updates.
    // So that the mouse cursor can be made visible again when moving the mouse.
    this->setMouseTracking(true);
    d->setMouseTracking(true);
}


VideoPlayer::~VideoPlayer()
{
    if (d->fPipeline) {
        ::currentVideo = 0;
        gst_bus_remove_signal_watch(d->fBus);
#if not GST_CHECK_VERSION(1, 0, 0)
        gst_bus_disable_sync_message_emission(d->fBus);
#endif
        gst_element_set_state(d->fPipeline, GST_STATE_NULL);
        gst_object_unref(d->fBus);
        gst_object_unref(d->fPipeline);
    }
    if (fRwops) {
        SDL_RWclose(fRwops);
    }
}


extern "C" {

#if not GST_CHECK_VERSION(1, 0, 0)
static void
cbSyncMessage(GstBus*, GstMessage* message, gpointer userData)
{
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ELEMENT
        and gst_structure_has_name(message->structure, "prepare-xwindow-id"))
    {
         GstElement* sink = GST_ELEMENT(GST_MESSAGE_SRC(message));
         gst_x_overlay_set_window_handle(GST_X_OVERLAY(GST_MESSAGE_SRC(message)),
                                         (guintptr)static_cast<QWidget*>(userData)->winId());
         g_object_set(sink, "force-aspect-ratio", true, NULL);
     }
}
#endif


static void
cbOnBusMessage(GstBus*, GstMessage* message, gpointer d)
{
    VideoPlayer_priv::cbOnBusMessage(message, static_cast<VideoPlayer_priv*>(d));
}


static void
cbOnSourceSetup(GstPipeline*, GstAppSrc* source, gpointer d)
{
    static_cast<VideoPlayer_priv*>(d)->cbOnSourceSetup(source, static_cast<VideoPlayer_priv*>(d));
}

} // extern "C"


bool
VideoPlayer::loadVideo(FILE* src, long len, bool loop)
{
    if (not d->fPipeline) {
        const char* playbinName =
#if GST_CHECK_VERSION(1, 0, 0)
                "playbin";
#else
                "playbin2";
#endif
        d->fPipeline = gst_element_factory_make(playbinName, 0);
        if (not d->fPipeline) {
            hMainWin->errorMsgObj()->showMessage(tr("Unable to play video. You are "
                                                    "probably missing the GStreamer plugins "
                                                    "from the \"gst-plugins-base\" set."));
            return false;
        }

        d->fBus = gst_pipeline_get_bus(GST_PIPELINE(d->fPipeline));
        // We need to be informed when the playback state changes.
        gst_bus_add_signal_watch(d->fBus);
        g_signal_connect(d->fBus, "message", G_CALLBACK(cbOnBusMessage), d);
#if GST_CHECK_VERSION(1, 0, 0)
        // With gst 1.x, we can configure aspect ratio and set the window ID
        // directly on playbin.
        g_object_set(d->fPipeline, "force-aspect-ratio", true, NULL);
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(d->fPipeline), (guintptr)d->winId());
#else
        // With gst 0.10, we need to be informed when a video sink is added
        // so we can configure it later.
        gst_bus_enable_sync_message_emission(d->fBus);
        g_signal_connect(d->fBus, "sync-message", G_CALLBACK(cbSyncMessage), d);
#endif
        g_signal_connect(d->fPipeline, "source-setup", G_CALLBACK(cbOnSourceSetup), d);
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
    g_object_set(G_OBJECT(d->fPipeline), "uri", "appsrc://", NULL);
    return true;
}


void
VideoPlayer::play()
{
    if (d->fPipeline == 0) {
        return;
    }

    hFrame->updateGameScreen(true);

    this->d->setMaximumSize(this->maximumSize());
    ::currentVideo = this;
    updateVolume();
    gst_element_set_state(d->fPipeline, GST_STATE_PLAYING);
    hApp->advanceEventLoop();
    if (this->fLooping) {
        // Wait for the pipeline to transition into the playing state.
        gst_element_get_state(d->fPipeline, 0, 0, GST_CLOCK_TIME_NONE);
        // Seek to the end the first time so that there's no pause before
        // the first segment message arrives.
        if (not gst_element_seek(d->fPipeline, 1.0, GST_FORMAT_TIME,
                                 (GstSeekFlags)((int)GST_SEEK_FLAG_FLUSH | (int)GST_SEEK_FLAG_SEGMENT),
                                 GST_SEEK_TYPE_END, 0, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        {
            qWarning() << "Sending initial video seek event failed.";
            stop();
        }
    }
}


void
VideoPlayer::stop()
{
    if (d->fPipeline) {
        gst_element_set_state(d->fPipeline, GST_STATE_NULL);
        emit videoFinished();
        hide();
    }
}


void
VideoPlayer::updateVolume()
{
    if (d->fPipeline) {
        setVolume(d->fVolume);
    }
}


void
VideoPlayer::setVolume(int vol)
{
    if (not d->fPipeline) {
        return;
    }
    if (vol < 0) {
        vol = 0;
    } else if (vol > 100) {
        vol = 100;
    }
    d->fVolume = vol;

    // Attenuate the result by the global volume setting. Use an exponential
    // volume scale. Use the second power instead of the third to be consistent
    // with the SDL audio volume.
    g_object_set(d->fPipeline, "volume",
                 std::pow(((gdouble)vol * hApp->settings()->soundVolume) / 10000.0, 2),
                 NULL);
}


void
VideoPlayer::setMute(bool mute)
{
    if (d->fPipeline) {
        g_object_set(d->fPipeline, "mute", mute, NULL);
    }
}

void
VideoPlayer::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    d->resize(e->size());
}
