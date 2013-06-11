#ifndef VIDEOPLAYERGST_P
#define VIDEOPLAYERGST_P

#include <QGst/Ui/VideoWidget>
#include <QGst/Pipeline>


class VideoPlayer_priv: public QGst::Ui::VideoWidget {
    Q_OBJECT

public:
    VideoPlayer_priv(QWidget *parent, class VideoPlayer* qPtr)
        : QGst::Ui::VideoWidget(parent),
          q(qPtr),
          fAppSrc(0)
    { }

    class VideoPlayer* q;
    QGst::PipelinePtr fPipeline;
    class RwopsApplicationSource* fAppSrc;

    void fSetupSource(QGst::ElementPtr source);
    void fOnBusMessage(const QGst::MessagePtr& message);
    void fOnVideoSinkChange(const QGst::MessagePtr& msg);

signals:
    void videoFinished();
    void errorOccurred();
};


#endif
