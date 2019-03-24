// This is copyrighted software. More information is at the end of this file.
#include "enginerunner.h"

#include <QThread>
#include <array>
#include <memory>

extern "C" {
#include "heheader.h"
}

void EngineRunner::runEngine()
{
    char argv0[] = "hugor";
    auto argv1 = std::make_unique<char[]>(gamefile_.toLocal8Bit().size() + 1);
    memcpy(argv1.get(), gamefile_.toLocal8Bit().constData(), gamefile_.toLocal8Bit().size() + 1);
    std::array<char*, 2> argv = {argv0, argv1.get()};
    EngineThread::setTerminationEnabled(true);
    he_main(2, argv.data());
    emit finished();
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
