// This is copyrighted software. More information is at the end of this file.
#pragma once
#ifndef DISABLE_VIDEO
#include <QWidget>

class VideoPlayer_priv;
struct HugorFile;
struct SDL_RWops;

class VideoPlayer final: public QWidget
{
    Q_OBJECT

public:
    VideoPlayer(QWidget* parent = nullptr);
    ~VideoPlayer() override;

    bool loadVideo(HugorFile* src, long len, bool loop);

public slots:
    void play();
    void stop();
    void updateVolume();
    void setVolume(int vol);
    void setMute(bool mute);

signals:
    void videoFinished();
    void errorOccurred();

protected:
    void resizeEvent(QResizeEvent* e) override;
    QPaintEngine* paintEngine() const override;

private:
    SDL_RWops* rwops_ = nullptr;
    long data_len = 0;
    bool is_looping = false;
    friend class VideoPlayer_priv;
    VideoPlayer_priv* d_ = nullptr;
};

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
