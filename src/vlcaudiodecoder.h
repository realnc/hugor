// This is copyrighted software. More information is at the end of this file.
#pragma once
#include "Aulib/AudioDecoder.h"

#include <QMutex>
#include <boost/circular_buffer.hpp>
#include <cstdint>

class VlcAudioDecoder final: public Aulib::AudioDecoder
{
public:
    void pushSamples(const void* samples, unsigned count) noexcept;
    void discardPendingSamples() noexcept;

    bool open(SDL_RWops* rwops) override;
    int getChannels() const override;
    int getRate() const override;
    bool rewind() override;
    std::chrono::microseconds duration() const override;
    bool seekToTime(std::chrono::microseconds pos) override;

protected:
    int doDecoding(float buf[], int len, bool& callAgain) override;

private:
    boost::circular_buffer<int16_t> sample_buf_{131072};
    QMutex sample_buf_mutex_;
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
