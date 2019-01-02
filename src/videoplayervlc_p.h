// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <memory>
#include <vlc/vlc.h>

#include "happlication.h"
#include "settings.h"

class VideoPlayer;

class VideoPlayer_priv final
{
public:
    VideoPlayer_priv(VideoPlayer* q_ptr)
        : q(q_ptr)
    {}

    VideoPlayer* q;
    std::unique_ptr<libvlc_instance_t, decltype(&libvlc_release)> vlc_instance {nullptr, nullptr};
    std::unique_ptr<libvlc_media_player_t, decltype(&libvlc_media_player_release)> vlc_player {
        nullptr, nullptr};
    int volume = hApp->settings()->sound_volume;
    bool is_muted = false;
    bool is_looping = false;
};

#ifdef DL_VLC
extern "C" {
extern decltype(&libvlc_media_add_option) libvlc_media_add_option_ptr;
extern decltype(&libvlc_media_event_manager) libvlc_media_event_manager_ptr;
extern decltype(&libvlc_media_new_callbacks) libvlc_media_new_callbacks_ptr;
extern decltype(&libvlc_media_player_event_manager) libvlc_media_player_event_manager_ptr;
extern decltype(&libvlc_media_player_is_playing) libvlc_media_player_is_playing_ptr;
extern decltype(&libvlc_media_player_new) libvlc_media_player_new_ptr;
extern decltype(&libvlc_media_player_play) libvlc_media_player_play_ptr;
extern decltype(&libvlc_media_player_release) libvlc_media_player_release_ptr;
extern decltype(&libvlc_media_player_set_hwnd) libvlc_media_player_set_hwnd_ptr;
extern decltype(&libvlc_media_player_set_media) libvlc_media_player_set_media_ptr;
extern decltype(&libvlc_media_player_stop) libvlc_media_player_stop_ptr;
extern decltype(&libvlc_media_release) libvlc_media_release_ptr;
extern decltype(&libvlc_new) libvlc_new_ptr;
extern decltype(&libvlc_release) libvlc_release_ptr;
extern decltype(&libvlc_video_get_size) libvlc_video_get_size_ptr;
extern decltype(&libvlc_video_set_key_input) libvlc_video_set_key_input_ptr;
extern decltype(&libvlc_video_set_mouse_input) libvlc_video_set_mouse_input_ptr;
extern decltype(&libvlc_event_attach) libvlc_event_attach_ptr;
extern decltype(&libvlc_media_player_set_nsobject) libvlc_media_player_set_nsobject_ptr;
extern decltype(&libvlc_media_player_set_xwindow) libvlc_media_player_set_xwindow_ptr;
}
#endif

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
