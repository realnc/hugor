#ifndef VIDEOPLAYERGST_P
#define VIDEOPLAYERGST_P

#include <QWidget>
#include <gst/gstelement.h>
#include <gst/gstpipeline.h>
#include <gst/app/gstappsrc.h>
#include <cstring>


class VideoPlayer_priv: public QWidget {
    Q_OBJECT

public:
    VideoPlayer_priv(QWidget *parent, class VideoPlayer* qPtr)
        : QWidget(parent),
          q(qPtr),
          fPipeline(0),
          fAppSrc(0)
    {
        memset(&fAppSrcCbs, 0, sizeof(fAppSrcCbs));
    }

    class VideoPlayer* q;
    GstElement* fPipeline;
    GstAppSrc* fAppSrc;
    GstAppSrcCallbacks fAppSrcCbs;

signals:
    void videoFinished();
    void errorOccurred();

public:
    static void cbOnSourceSetup(GstPipeline*, GstAppSrc* source, VideoPlayer_priv* d);
    static void cbOnBusMessage(GstBus*, GstMessage* message, VideoPlayer_priv* d);
};


#endif
