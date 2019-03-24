// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QThread>

class EngineThread final: public QThread
{
    Q_OBJECT

public:
    friend class EngineRunner;

    explicit EngineThread(QObject* parent = nullptr)
        : QThread(parent)
    {}
};

class EngineRunner final: public QObject
{
    Q_OBJECT

public:
    EngineRunner(QString gameFile, EngineThread* thread, QObject* parent = nullptr)
        : QObject(parent)
        , thread_(thread)
        , gamefile_(std::move(gameFile))
    {}

signals:
    void finished();

public slots:
    void runEngine();

private:
    EngineThread* thread_;
    QString gamefile_;
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
