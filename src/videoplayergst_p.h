#ifndef VIDEOPLAYERGST_P
#define VIDEOPLAYERGST_P

#include <QWidget>
#include <gst/gstelement.h>
#include <gst/gstpipeline.h>
#include <gst/app/gstappsrc.h>


class VideoPlayer_priv: public QWidget {
    Q_OBJECT

public:
    VideoPlayer_priv(QWidget *parent, class VideoPlayer* qPtr);
    class VideoPlayer* q;
    GstElement* fPipeline;
    GstAppSrc* fAppSrc;
    GstBus* fBus;
    GstAppSrcCallbacks fAppSrcCbs;
    static GMainLoop* fGMainLoop;
    static GThread* fGMainLoopThread;

    static void cbOnSourceSetup(GstAppSrc* source, VideoPlayer_priv* d);
    static void cbOnBusMessage(GstMessage* message, VideoPlayer_priv* d);

signals:
    void videoFinished();
    void errorOccurred();

public slots:
    void adjustForVideoSize(QSize vidSize);
};


#endif
