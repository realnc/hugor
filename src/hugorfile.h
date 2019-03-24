// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <cstdio>

struct HugorFile final
{
public:
    explicit HugorFile(FILE* const handle) noexcept
        : handle_(handle)
    {}

    ~HugorFile() noexcept
    {
        close();
    }

    HugorFile(const HugorFile&) = delete;
    HugorFile& operator=(const HugorFile&) = delete;

    FILE* get() const noexcept
    {
        return handle_;
    }

    FILE* release() noexcept
    {
        auto* tmp = handle_;
        handle_ = nullptr;
        return tmp;
    }

    int close() noexcept
    {
        if (handle_ == nullptr) {
            return 0;
        }
        auto ret = std::fclose(handle_);
        handle_ = nullptr;
        return ret;
    }

private:
    FILE* handle_;
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
