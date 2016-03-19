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
#ifndef HUGOHANDLERS_H
#define HUGOHANDLERS_H

#include <QObject>
extern "C" {
#include "heheader.h"
}


extern class HugoHandlers* hHandlers;


/*
 * Handles hugo callbacks from heqt.cc that we want executed in the main thread.
 * The hugo engine runs in a separate thread, so the heqt.cc callbacks will
 * delegate some work to us in order to ensure it runs in the main thrad (like
 * GUI operations.)
 */
class HugoHandlers: public QObject {
    Q_OBJECT

  public:
    HugoHandlers(QObject* parent = 0)
        : QObject(parent),
          fVidPlayer(0)
    { }

    void calcFontDimensions();

  public slots:
    void getfilename(char* a, char* b);
    void startGetline(char* p);
    void endGetline();
    void clearfullscreen();
    void clearwindow();
    void settextmode();
    void settextwindow(int left, int top, int right, int bottom);
    void printFatalError(char* a);
    void print(char* a);
    void font(int f);
    void settextcolor(int c);
    void setbackcolor(int c);
    void displaypicture(HUGO_FILE infile, long len, int* result);
    void playmusic(HUGO_FILE infile, long reslength, char loop_flag, int* result);
    void musicvolume(int vol);
    void stopmusic();
    void playsample(HUGO_FILE infile, long reslength, char loop_flag, int* result);
    void samplevolume(int vol);
    void stopsample();
#ifndef DISABLE_VIDEO
    void stopvideo();
    void playvideo(HUGO_FILE infile, long len, char loop, char bg, int vol, int* result);
#endif

    void fadeScreen(int durationMs, int startAlpha, int endAlpha, bool block);

  private:
    class VideoPlayer* fVidPlayer;
};


#endif
