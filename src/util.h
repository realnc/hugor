// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QApplication>
#include <QtGlobal>
#include <type_traits>

template<typename F>
static void runInMainThread(F&& fun)
{
    QObject tmp;
    QObject::connect(&tmp, &QObject::destroyed, qApp, std::forward<F>(fun),
                     Qt::BlockingQueuedConnection);
}

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
template<typename T>
constexpr typename std::add_const<T>::type& qAsConst(T& t) noexcept
{
    return t;
}

template<typename T>
void qAsConst(const T&&) = delete;
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
 */
