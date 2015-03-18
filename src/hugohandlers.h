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
    int displaypicture(HUGO_FILE infile, long len);
    int playmusic(HUGO_FILE infile, long reslength, char loop_flag);
    void musicvolume(int vol);
    void stopmusic();
    int playsample(HUGO_FILE infile, long reslength, char loop_flag);
    void samplevolume(int vol);
    void stopsample();
#ifndef DISABLE_VIDEO
    void stopvideo();
    int playvideo(HUGO_FILE infile, long len, char loop, char bg, int vol);
#endif

  private:
    class VideoPlayer* fVidPlayer;
};


#endif
