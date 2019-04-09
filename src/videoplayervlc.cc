// This is copyrighted software. More information is at the end of this file.
#include "hugodefs.h"
#include "videoplayer.h"

#include <QDebug>
#include <QErrorMessage>
#include <QLibrary>
#include <QResizeEvent>
#include <SDL_rwops.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <vlc/vlc.h>

#include "Aulib/Stream.h"
#include "happlication.h"
#include "hmainwindow.h"
#include "hugorfile.h"
#include "rwopsbundle.h"
#include "settings.h"
#include "util.h"
#include "videoplayervlc_p.h"
#include "vlcaudiodecoder.h"

#ifdef DL_VLC
extern "C" {
static decltype(&libvlc_audio_set_callbacks) libvlc_audio_set_callbacks_ptr;
#define libvlc_audio_set_callbacks libvlc_audio_set_callbacks_ptr
static decltype(&libvlc_audio_set_delay) libvlc_audio_set_delay_ptr;
#define libvlc_audio_set_delay libvlc_audio_set_delay_ptr
static decltype(&libvlc_audio_set_format) libvlc_audio_set_format_ptr;
#define libvlc_audio_set_format libvlc_audio_set_format_ptr
static decltype(&libvlc_errmsg) libvlc_errmsg_ptr;
#define libvlc_errmsg libvlc_errmsg_ptr
static decltype(&libvlc_event_attach) libvlc_event_attach_ptr;
#define libvlc_event_attach libvlc_event_attach_ptr
static decltype(&libvlc_media_add_option) libvlc_media_add_option_ptr;
#define libvlc_media_add_option libvlc_media_add_option_ptr
static decltype(&libvlc_media_event_manager) libvlc_media_event_manager_ptr;
#define libvlc_media_event_manager libvlc_media_event_manager_ptr
static decltype(&libvlc_media_new_callbacks) libvlc_media_new_callbacks_ptr;
#define libvlc_media_new_callbacks libvlc_media_new_callbacks_ptr
static decltype(&libvlc_media_player_event_manager) libvlc_media_player_event_manager_ptr;
#define libvlc_media_player_event_manager libvlc_media_player_event_manager_ptr
static decltype(&libvlc_media_player_is_playing) libvlc_media_player_is_playing_ptr;
#define libvlc_media_player_is_playing libvlc_media_player_is_playing_ptr
static decltype(&libvlc_media_player_new) libvlc_media_player_new_ptr;
#define libvlc_media_player_new libvlc_media_player_new_ptr
static decltype(&libvlc_media_player_play) libvlc_media_player_play_ptr;
#define libvlc_media_player_play libvlc_media_player_play_ptr
static decltype(&libvlc_media_player_release) libvlc_media_player_release_ptr;
#define libvlc_media_player_release libvlc_media_player_release_ptr
static decltype(&libvlc_media_player_set_hwnd) libvlc_media_player_set_hwnd_ptr;
#define libvlc_media_player_set_hwnd libvlc_media_player_set_hwnd_ptr
static decltype(&libvlc_media_player_set_media) libvlc_media_player_set_media_ptr;
#define libvlc_media_player_set_media libvlc_media_player_set_media_ptr
static decltype(&libvlc_media_player_set_nsobject) libvlc_media_player_set_nsobject_ptr;
#define libvlc_media_player_set_nsobject libvlc_media_player_set_nsobject_ptr
static decltype(&libvlc_media_player_set_xwindow) libvlc_media_player_set_xwindow_ptr;
#define libvlc_media_player_set_xwindow libvlc_media_player_set_xwindow_ptr
static decltype(&libvlc_media_player_stop) libvlc_media_player_stop_ptr;
#define libvlc_media_player_stop libvlc_media_player_stop_ptr
static decltype(&libvlc_media_release) libvlc_media_release_ptr;
#define libvlc_media_release libvlc_media_release_ptr
static decltype(&libvlc_new) libvlc_new_ptr;
#define libvlc_new libvlc_new_ptr
static decltype(&libvlc_release) libvlc_release_ptr;
#define libvlc_release libvlc_release_ptr
static decltype(&libvlc_video_get_size) libvlc_video_get_size_ptr;
#define libvlc_video_get_size libvlc_video_get_size_ptr
static decltype(&libvlc_video_set_key_input) libvlc_video_set_key_input_ptr;
#define libvlc_video_set_key_input libvlc_video_set_key_input_ptr
static decltype(&libvlc_video_set_mouse_input) libvlc_video_set_mouse_input_ptr;
#define libvlc_video_set_mouse_input libvlc_video_set_mouse_input_ptr
}
#endif

static VideoPlayer* video_player;

extern "C" {
static int vlcOpenCb(void* rwops, void** datap, uint64_t* sizep)
{
    *datap = rwops;
    *sizep = SDL_RWsize(static_cast<SDL_RWops*>(rwops));
    return 0;
}

static ssize_t vlcReadCb(void* rwops, unsigned char* buf, size_t len)
{
    auto ret = SDL_RWread(static_cast<SDL_RWops*>(rwops), buf, 1, len);
    return ret;
}

static int vlcSeekCb(void* rwops, uint64_t offset)
{
    if (SDL_RWseek(static_cast<SDL_RWops*>(rwops), offset, RW_SEEK_SET) < 0) {
        return -1;
    }
    return 0;
}

static void vlcCloseCb(void* rwops)
{
    SDL_RWclose(static_cast<SDL_RWops*>(rwops));
}

static void vlcMediaEndCb(const libvlc_event_t* /*event*/, void* videoplayer)
{
    runInMainThread([videoplayer] {
        auto* q = static_cast<VideoPlayer*>(videoplayer);
        q->hide();
        emit q->videoFinished();
    });
}

static void vlcMediaParsedCb(const libvlc_event_t* /*event*/, void* videoplayer_priv)
{
    runInMainThread([videoplayer_priv] {
        auto* d = static_cast<VideoPlayer_priv*>(videoplayer_priv);
        auto* q = d->q;
        unsigned width, height;
        if (libvlc_video_get_size(d->vlc_player.get(), 0, &width, &height) != 0) {
            return;
        }
        QSize vidSize(width, height);
        if (vidSize.width() > q->maximumWidth() or vidSize.height() > q->maximumHeight()) {
            vidSize.scale(q->maximumSize(), Qt::KeepAspectRatio);
        }
        q->setGeometry(q->x() + (q->maximumWidth() - vidSize.width()) / 2,
                       q->y() + (q->maximumHeight() - vidSize.height()) / 2, vidSize.width(),
                       vidSize.height());
        q->show();

        libvlc_audio_set_delay(d->vlc_player.get(),
                               ((float)Aulib::frameSize() / Aulib::sampleRate()) * -1000000);
    });
}

static void vlcAudioPlayCb(void* data, const void* samples, unsigned count, int64_t /*pts*/)
{
    static_cast<VideoPlayer_priv*>(data)->audio_decoder->pushSamples(samples, count);
}

static void vlcAudioFlushCb(void* data, int64_t /*pts*/)
{
    static_cast<VideoPlayer_priv*>(data)->audio_decoder->discardPendingSamples();
}
}

void initVideoEngine(int& /*argc*/, char* /*argv*/[])
{
#ifdef DL_VLC
    QLibrary lib("libvlc");
    if (not lib.load()) {
        hApp->settings()->video_sys_error = true;
        return;
    }
    libvlc_audio_set_callbacks_ptr =
        (decltype(libvlc_audio_set_callbacks_ptr))lib.resolve("libvlc_audio_set_callbacks");
    libvlc_audio_set_delay_ptr =
        (decltype(libvlc_audio_set_delay_ptr))lib.resolve("libvlc_audio_set_delay");
    libvlc_audio_set_format_ptr =
        (decltype(libvlc_audio_set_format_ptr))lib.resolve("libvlc_audio_set_format");
    libvlc_errmsg_ptr = (decltype(libvlc_errmsg_ptr))lib.resolve("libvlc_errmsg_ptr");
    libvlc_event_attach_ptr = (decltype(libvlc_event_attach_ptr))lib.resolve("libvlc_event_attach");
    libvlc_media_add_option_ptr =
        (decltype(libvlc_media_add_option_ptr))lib.resolve("libvlc_media_add_option");
    libvlc_media_event_manager_ptr =
        (decltype(libvlc_media_event_manager_ptr))lib.resolve("libvlc_media_event_manager");
    libvlc_media_new_callbacks_ptr =
        (decltype(libvlc_media_new_callbacks_ptr))lib.resolve("libvlc_media_new_callbacks");
    libvlc_media_player_event_manager_ptr = (decltype(
        libvlc_media_player_event_manager_ptr))lib.resolve("libvlc_media_player_event_manager");
    libvlc_media_player_is_playing_ptr =
        (decltype(libvlc_media_player_is_playing_ptr))lib.resolve("libvlc_media_player_is_playing");
    libvlc_media_player_new_ptr =
        (decltype(libvlc_media_player_new_ptr))lib.resolve("libvlc_media_player_new");
    libvlc_media_player_play_ptr =
        (decltype(libvlc_media_player_play_ptr))lib.resolve("libvlc_media_player_play");
    libvlc_media_player_release_ptr =
        (decltype(libvlc_media_player_release_ptr))lib.resolve("libvlc_media_player_release");
    libvlc_media_player_set_hwnd_ptr =
        (decltype(libvlc_media_player_set_hwnd_ptr))lib.resolve("libvlc_media_player_set_hwnd");
    libvlc_media_player_set_media_ptr =
        (decltype(libvlc_media_player_set_media_ptr))lib.resolve("libvlc_media_player_set_media");
    libvlc_media_player_set_nsobject_ptr = (decltype(
        libvlc_media_player_set_nsobject_ptr))lib.resolve("libvlc_media_player_set_nsobject");
    libvlc_media_player_set_xwindow_ptr = (decltype(
        libvlc_media_player_set_xwindow_ptr))lib.resolve("libvlc_media_player_set_xwindow");
    libvlc_media_player_stop_ptr =
        (decltype(libvlc_media_player_stop_ptr))lib.resolve("libvlc_media_player_stop");
    libvlc_media_release_ptr =
        (decltype(libvlc_media_release_ptr))lib.resolve("libvlc_media_release");
    libvlc_new_ptr = (decltype(libvlc_new_ptr))lib.resolve("libvlc_new");
    libvlc_release_ptr = (decltype(libvlc_release_ptr))lib.resolve("libvlc_release");
    libvlc_video_get_size_ptr =
        (decltype(libvlc_video_get_size_ptr))lib.resolve("libvlc_video_get_size");
    libvlc_video_set_key_input_ptr =
        (decltype(libvlc_video_set_key_input_ptr))lib.resolve("libvlc_video_set_key_input");
    libvlc_video_set_mouse_input_ptr =
        (decltype(libvlc_video_set_mouse_input_ptr))lib.resolve("libvlc_video_set_mouse_input");
#endif
}

void closeVideoEngine()
{
    if (video_player != nullptr) {
        video_player->stop();
    }
}

void muteVideo(bool mute)
{
    if (video_player != nullptr) {
        video_player->setMute(mute);
    }
}

void updateVideoVolume()
{
    if (video_player != nullptr) {
        video_player->updateVolume();
    }
}

VideoPlayer::VideoPlayer(QWidget* parent)
    : QWidget(parent)
    , d_(new VideoPlayer_priv(this))
{
    setAttribute(Qt::WA_NativeWindow);

    d_->vlc_instance = {libvlc_new(0, nullptr), libvlc_release};

    if (not d_->vlc_instance) {
        QString msg(QLatin1String("Failed to initialize LibVLC"));
        if (libvlc_errmsg() == nullptr) {
            msg += '.';
        } else {
            msg += QLatin1String(": ") + libvlc_errmsg();
        }
        hApp->settings()->video_sys_error = true;
        hMainWin->errorMsgObj()->showMessage(msg);
        return;
    }

    d_->vlc_player = {libvlc_media_player_new(d_->vlc_instance.get()), libvlc_media_player_release};
    if (not d_->vlc_player) {
        QString msg(QLatin1String("Failed to create LibVLC player"));
        if (libvlc_errmsg() == nullptr) {
            msg += '.';
        } else {
            msg += QLatin1String(": ") + libvlc_errmsg();
        }
        hApp->settings()->video_sys_error = true;
        hMainWin->errorMsgObj()->showMessage(msg);
        return;
    }

    auto decoder = std::make_unique<VlcAudioDecoder>();
    d_->audio_decoder = decoder.get();
    d_->audio_stream = std::make_unique<Aulib::Stream>(nullptr, std::move(decoder), false);
    d_->audio_stream->play();

    // We request 16-bit int instead of float samples because current libVLC (3.0.6) is bugged; it
    // ignores the sample format request and always feeds us 16-bit int samples.
    libvlc_audio_set_format(d_->vlc_player.get(), "S16N", Aulib::sampleRate(),
                            Aulib::channelCount());
    libvlc_audio_set_callbacks(d_->vlc_player.get(), vlcAudioPlayCb, nullptr, nullptr,
                               vlcAudioFlushCb, nullptr, d_);

    libvlc_video_set_key_input(d_->vlc_player.get(), false);
    libvlc_video_set_mouse_input(d_->vlc_player.get(), false);

    libvlc_event_attach(libvlc_media_player_event_manager(d_->vlc_player.get()),
                        libvlc_MediaPlayerEndReached, vlcMediaEndCb, this);

    setAttribute(Qt::WA_PaintOnScreen, true);
    setMouseTracking(true);
    Q_ASSERT(video_player == nullptr);
    video_player = this;
}

VideoPlayer::~VideoPlayer()
{
    delete d_;
    video_player = nullptr;
}

bool VideoPlayer::loadVideo(HugorFile* src, long len, bool loop)
{
    if (d_->vlc_instance == nullptr or d_->vlc_player == nullptr) {
        return false;
    }
    if (libvlc_media_player_is_playing(d_->vlc_player.get())) {
        stop();
    }

    SDL_ClearError();
    auto* rwops = RWFromMediaBundle(src->get(), len);
    if (rwops == nullptr) {
        QString msg(QLatin1String("Failed to open video data"));
        if (strlen(SDL_GetError()) == 0) {
            msg += '.';
        } else {
            msg += QLatin1String(": ") + SDL_GetError();
        }
        hMainWin->errorMsgObj()->showMessage(msg);
        return false;
    }
    src->release();

    auto* media = libvlc_media_new_callbacks(d_->vlc_instance.get(), vlcOpenCb, vlcReadCb,
                                             vlcSeekCb, vlcCloseCb, rwops);
    if (media == nullptr) {
        QString msg(QLatin1String("Failed to create LibVLC media"));
        if (libvlc_errmsg() == nullptr) {
            msg += '.';
        } else {
            msg += QLatin1String(": ") + libvlc_errmsg();
        }
        hMainWin->errorMsgObj()->showMessage(msg);
        SDL_RWclose(rwops);
        return false;
    }

    if (loop) {
        libvlc_media_add_option(media, "input-repeat=65535");
    }
    libvlc_media_player_set_media(d_->vlc_player.get(), media);
    libvlc_media_release(media);
#if defined(Q_OS_MAC)
    libvlc_media_player_set_nsobject(d_->vlc_player.get(), (void*)winId());
#elif defined(Q_OS_UNIX)
    libvlc_media_player_set_xwindow(d_->vlc_player.get(), winId());
#elif defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(d_->vlc_player.get(), (void*)winId());
#endif
    libvlc_event_attach(libvlc_media_event_manager(media), libvlc_MediaParsedChanged,
                        vlcMediaParsedCb, d_);
    d_->is_looping = loop;
    return true;
}

void VideoPlayer::play()
{
    parentWidget()->update();
    update();
    d_->audio_decoder->discardPendingSamples();
    libvlc_media_player_play(d_->vlc_player.get());
}

void VideoPlayer::stop()
{
    if (d_->vlc_instance == nullptr or d_->vlc_player == nullptr) {
        return;
    }
    libvlc_media_player_stop(d_->vlc_player.get());
    hide();
    emit videoFinished();
}

void VideoPlayer::updateVolume()
{
    if (d_->vlc_player == nullptr) {
        return;
    }
    d_->audio_stream->setVolume(
        std::pow((d_->hugo_volume / 100.f) * (hApp->settings()->sound_volume / 100.f), 2.f));
}

void VideoPlayer::setVolume(const int vol)
{
    d_->hugo_volume = std::min(std::max(0, vol), 100);
    updateVolume();
}

void VideoPlayer::setMute(const bool mute)
{
    if (mute) {
        d_->audio_stream->mute();
    } else {
        d_->audio_stream->unmute();
    }
}

void VideoPlayer::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
}

QPaintEngine* VideoPlayer::paintEngine() const
{
    return nullptr;
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
