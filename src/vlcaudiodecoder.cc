// This is copyrighted software. More information is at the end of this file.
#include "vlcaudiodecoder.h"

#include "aulib.h"
#include <QDebug>
#include <QMutexLocker>
#include <algorithm>

void VlcAudioDecoder::pushSamples(const void* samples, unsigned count) noexcept
{
    Q_ASSERT(samples != nullptr);

    const auto* begin = static_cast<const int16_t*>(samples);
    const auto* end = begin + count * getChannels();

    QMutexLocker lock(&sample_buf_mutex_);
    size_t needed_capacity = sample_buf_.size() + count * getChannels();
    if (sample_buf_.capacity() < needed_capacity) {
        sample_buf_.set_capacity(std::max(needed_capacity, sample_buf_.capacity() * 2));
    }
    sample_buf_.insert(sample_buf_.end(), begin, end);
}

void VlcAudioDecoder::discardPendingSamples() noexcept
{
    QMutexLocker lock(&sample_buf_mutex_);
    sample_buf_.clear();
}

bool VlcAudioDecoder::open(SDL_RWops* rwops)
{
    Q_ASSERT(rwops == nullptr);
    return true;
}

int VlcAudioDecoder::getChannels() const
{
    return Aulib::channelCount();
}

int VlcAudioDecoder::getRate() const
{
    return Aulib::sampleRate();
}

bool VlcAudioDecoder::rewind()
{
    return false;
}

std::chrono::microseconds VlcAudioDecoder::duration() const
{
    return {};
}

bool VlcAudioDecoder::seekToTime(std::chrono::microseconds)
{
    return false;
}

int VlcAudioDecoder::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;

    QMutexLocker locker(&sample_buf_mutex_);
    int count = std::min(len, (int)sample_buf_.size());
    for (int i = 0; i < count; ++i) {
        buf[i] = sample_buf_[i] / 32768.f;
    }
    sample_buf_.erase_begin(count);
    locker.unlock();

    memset(buf + count, 0, (len - count) * sizeof(*buf));
    return len;
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
