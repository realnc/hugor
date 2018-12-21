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
