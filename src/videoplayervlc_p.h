// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <memory>
#include <vlc/vlc.h>

namespace Aulib {
class Stream;
}
class VideoPlayer;
class VlcAudioDecoder;

class VideoPlayer_priv final
{
public:
    VideoPlayer_priv(VideoPlayer* q_ptr)
        : q(q_ptr)
    {}

    VideoPlayer* q;
    std::unique_ptr<libvlc_instance_t, decltype(&libvlc_release)> vlc_instance{nullptr, nullptr};
    std::unique_ptr<libvlc_media_player_t, decltype(&libvlc_media_player_release)> vlc_player{
        nullptr, nullptr};
    std::unique_ptr<Aulib::Stream> audio_stream;
    VlcAudioDecoder* audio_decoder = nullptr;
    int hugo_volume = 100;
    bool is_looping = false;
};

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
