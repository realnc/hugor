// This is copyrighted software. More information is at the end of this file.
#include "hugodefs.h"
#include "videoplayer.h"

#include <QDebug>
#include <QErrorMessage>
#include <QLibrary>
#include <QResizeEvent>
#include <QThread>
#include <SDL_rwops.h>
#include <algorithm>
#include <memory>
#include <vlc/vlc.h>

#include "happlication.h"
#include "hmainwindow.h"
#include "hugorfile.h"
#include "rwopsbundle.h"
#include "settings.h"
#include "util.h"
#include "videoplayervlc_p.h"

#ifdef DL_VLC
decltype(&libvlc_media_add_option) libvlc_media_add_option_ptr = nullptr;
decltype(&libvlc_media_event_manager) libvlc_media_event_manager_ptr = nullptr;
decltype(&libvlc_media_new_callbacks) libvlc_media_new_callbacks_ptr = nullptr;
decltype(&libvlc_media_player_event_manager) libvlc_media_player_event_manager_ptr = nullptr;
decltype(&libvlc_media_player_is_playing) libvlc_media_player_is_playing_ptr = nullptr;
decltype(&libvlc_media_player_new) libvlc_media_player_new_ptr = nullptr;
decltype(&libvlc_media_player_play) libvlc_media_player_play_ptr = nullptr;
decltype(&libvlc_media_player_release) libvlc_media_player_release_ptr = nullptr;
decltype(&libvlc_media_player_set_hwnd) libvlc_media_player_set_hwnd_ptr = nullptr;
decltype(&libvlc_media_player_set_media) libvlc_media_player_set_media_ptr = nullptr;
decltype(&libvlc_media_player_stop) libvlc_media_player_stop_ptr = nullptr;
decltype(&libvlc_media_release) libvlc_media_release_ptr = nullptr;
decltype(&libvlc_new) libvlc_new_ptr = nullptr;
decltype(&libvlc_release) libvlc_release_ptr = nullptr;
decltype(&libvlc_video_get_size) libvlc_video_get_size_ptr = nullptr;
decltype(&libvlc_video_set_key_input) libvlc_video_set_key_input_ptr = nullptr;
decltype(&libvlc_video_set_mouse_input) libvlc_video_set_mouse_input_ptr = nullptr;
decltype(&libvlc_event_attach) libvlc_event_attach_ptr = nullptr;
decltype(&libvlc_media_player_set_nsobject) libvlc_media_player_set_nsobject_ptr = nullptr;
decltype(&libvlc_media_player_set_xwindow) libvlc_media_player_set_xwindow_ptr = nullptr;
decltype(&libvlc_errmsg) libvlc_errmsg_ptr = nullptr;
decltype(&libvlc_audio_set_volume) libvlc_audio_set_volume_ptr = nullptr;
#include "dlvlcdef.h"
#endif

static VideoPlayer* video_player = nullptr;

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
    });
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
    libvlc_event_attach_ptr = (decltype(libvlc_event_attach_ptr))lib.resolve("libvlc_event_attach");
    libvlc_video_get_size_ptr =
        (decltype(libvlc_video_get_size_ptr))lib.resolve("libvlc_video_get_size");
    libvlc_new_ptr = (decltype(libvlc_new_ptr))lib.resolve("libvlc_new");
    libvlc_release_ptr = (decltype(libvlc_release_ptr))lib.resolve("libvlc_release");
    libvlc_media_player_new_ptr =
        (decltype(libvlc_media_player_new_ptr))lib.resolve("libvlc_media_player_new");
    libvlc_media_player_release_ptr =
        (decltype(libvlc_media_player_release_ptr))lib.resolve("libvlc_media_player_release");
    libvlc_video_set_key_input_ptr =
        (decltype(libvlc_video_set_key_input_ptr))lib.resolve("libvlc_video_set_key_input");
    libvlc_video_set_mouse_input_ptr =
        (decltype(libvlc_video_set_mouse_input_ptr))lib.resolve("libvlc_video_set_mouse_input");
    libvlc_media_player_event_manager_ptr = (decltype(
        libvlc_media_player_event_manager_ptr))lib.resolve("libvlc_media_player_event_manager");
    libvlc_media_player_play_ptr =
        (decltype(libvlc_media_player_play_ptr))lib.resolve("libvlc_media_player_play");
    libvlc_media_player_stop_ptr =
        (decltype(libvlc_media_player_stop_ptr))lib.resolve("libvlc_media_player_stop");
    libvlc_media_player_is_playing_ptr =
        (decltype(libvlc_media_player_is_playing_ptr))lib.resolve("libvlc_media_player_is_playing");
    libvlc_media_new_callbacks_ptr =
        (decltype(libvlc_media_new_callbacks_ptr))lib.resolve("libvlc_media_new_callbacks");
    libvlc_media_player_set_media_ptr =
        (decltype(libvlc_media_player_set_media_ptr))lib.resolve("libvlc_media_player_set_media");
    libvlc_media_player_set_hwnd_ptr =
        (decltype(libvlc_media_player_set_hwnd_ptr))lib.resolve("libvlc_media_player_set_hwnd");
    libvlc_media_event_manager_ptr =
        (decltype(libvlc_media_event_manager_ptr))lib.resolve("libvlc_media_event_manager");
    libvlc_media_release_ptr =
        (decltype(libvlc_media_release_ptr))lib.resolve("libvlc_media_release");
    libvlc_media_add_option_ptr =
        (decltype(libvlc_media_add_option_ptr))lib.resolve("libvlc_media_add_option");
    libvlc_media_player_set_xwindow_ptr = (decltype(
        libvlc_media_player_set_xwindow_ptr))lib.resolve("libvlc_media_player_set_xwindow");
    libvlc_errmsg_ptr = (decltype(libvlc_errmsg_ptr))lib.resolve("libvlc_errmsg_ptr");
    libvlc_audio_set_volume_ptr =
        (decltype(libvlc_audio_set_volume))lib.resolve("libvlc_audio_set_volume");
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
    libvlc_video_set_key_input(d_->vlc_player.get(), false);
    libvlc_video_set_mouse_input(d_->vlc_player.get(), false);
    auto* event_mgr = libvlc_media_player_event_manager(d_->vlc_player.get());
    libvlc_event_attach(event_mgr, libvlc_MediaPlayerEndReached, vlcMediaEndCb, this);
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
    setVolume(hApp->settings()->sound_volume);
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
    setVolume(hApp->settings()->sound_volume);
}

void VideoPlayer::setVolume(const int vol)
{
    d_->volume = std::min(std::max(0, vol), 100);
    if (d_->vlc_player == nullptr or d_->is_muted) {
        return;
    }
    libvlc_audio_set_volume(d_->vlc_player.get(), d_->volume);
}

void VideoPlayer::setMute(const bool mute)
{
    if (mute == d_->is_muted) {
        return;
    }
    d_->is_muted = mute;
    if (d_->vlc_player == nullptr) {
        return;
    }
    libvlc_audio_set_volume(d_->vlc_player.get(), mute ? 0 : d_->volume);
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
