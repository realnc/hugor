// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QIODevice>

struct SDL_RWops;

class RwopsQIODevice final: public QIODevice
{
    Q_OBJECT

public:
    explicit RwopsQIODevice(QObject* parent)
        : QIODevice(parent)
    {}

    bool atEnd() const override;
    bool isSequential() const override;
    bool seek(qint64 pos) override;
    qint64 size() const override;
    void close() override;

    using QIODevice::open;
    bool open(SDL_RWops* rwops, OpenMode mode);

protected:
    qint64 readData(char* data, qint64 len) override;
    qint64 writeData(const char* /*data*/, qint64 /*len*/) override;

private:
    struct SDL_RWops* rwops_ = nullptr;
    long size_ = 0;
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
