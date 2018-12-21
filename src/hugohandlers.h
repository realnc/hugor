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
#include <QObject>

extern "C" {
#include "heheader.h"
}

class VideoPlayer;
class HugoHandlers;

extern HugoHandlers* hHandlers;

/*
 * Handles hugo callbacks from heqt.cc that we want executed in the main thread. The hugo engine
 * runs in a separate thread, so the heqt.cc callbacks will delegate some work to us in order to
 * ensure it runs in the main thrad (like GUI operations.)
 */
class HugoHandlers final: public QObject
{
    Q_OBJECT

public:
    HugoHandlers(QObject* parent = nullptr)
        : QObject(parent)
    {}

    void calcFontDimensions() const;

public slots:
    void getfilename(char* a, char* b) const;
    void startGetline(char* p) const;
    void endGetline() const;
    void clearfullscreen() const;
    void clearwindow() const;
    void settextmode() const;
    void settextwindow(int left, int top, int right, int bottom) const;
    void printFatalError(char* a) const;
    void print(char* a) const;
    void font(int f) const;
    void settextcolor(int c) const;
    void setbackcolor(int c) const;
    void displaypicture(HUGO_FILE infile, long len, int* result) const;
    void playmusic(HUGO_FILE infile, long reslength, char loop_flag, int* result) const;
    void musicvolume(int vol) const;
    void stopmusic() const;
    void playsample(HUGO_FILE infile, long reslength, char loop_flag, int* result) const;
    void samplevolume(int vol) const;
    void stopsample() const;
#ifndef DISABLE_VIDEO
    void stopvideo() const;
    void playvideo(HUGO_FILE infile, long len, char loop, char bg, int vol, int* result);
#endif

private:
    VideoPlayer* vid_player_ = nullptr;
};
