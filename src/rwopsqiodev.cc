// This is copyrighted software. More information is at the end of this file.
#include "rwopsqiodev.h"

#include <SDL_rwops.h>

bool RwopsQIODevice::atEnd() const
{
    return (rwops_ != nullptr) and (size_ == SDL_RWtell(rwops_));
}

bool RwopsQIODevice::isSequential() const
{
    return (rwops_ != nullptr) and (rwops_->seek != nullptr);
}

bool RwopsQIODevice::seek(qint64 pos)
{
    if (rwops_ == nullptr) {
        return false;
    }
    QIODevice::seek(pos);
    long newPos = SDL_RWseek(rwops_, pos, RW_SEEK_SET);
    return newPos == pos;
}

qint64 RwopsQIODevice::size() const
{
    if (rwops_ != nullptr) {
        return size_;
    }
    return 0;
}

void RwopsQIODevice::close()
{
    rwops_ = nullptr;
    size_ = 0;
    QIODevice::close();
}

bool RwopsQIODevice::open(SDL_RWops* rwops, QIODevice::OpenMode mode)
{
    if (rwops == nullptr) {
        return false;
    }
    long pos = SDL_RWtell(rwops);
    long siz = SDL_RWseek(rwops, 0, RW_SEEK_END) - SDL_RWseek(rwops, 0, RW_SEEK_SET);
    SDL_RWseek(rwops, pos, RW_SEEK_SET);
    size_ = siz;
    rwops_ = rwops;
    return QIODevice::open(mode);
}

qint64 RwopsQIODevice::readData(char* data, qint64 len)
{
    if (rwops_ == nullptr) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }
    long cnt = SDL_RWread(rwops_, data, 1, len);
    if (cnt == 0) {
        return -1;
    }
    return cnt;
}

qint64 RwopsQIODevice::writeData(const char* /*data*/, qint64 /*len*/)
{
    return -1;
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
