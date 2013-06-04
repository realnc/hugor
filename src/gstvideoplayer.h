#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QGst/Ui/VideoWidget>
#include <QGst/Pipeline>


class VideoPlayer: public QGst::Ui::VideoWidget {
    Q_OBJECT

public:
    VideoPlayer( QWidget* parent = 0 );
    ~VideoPlayer();

    bool loadVideo(FILE* src, long len, bool loop);

public slots:
    void play();
    void stop();
    void setVolume(int vol);

signals:
    void videoFinished();
    void errorOccured();

private:
    void fSetupSource(QGst::ElementPtr source);
    void fOnBusMessage(const QGst::MessagePtr& message);
    void fOnVideoSinkChange(const QGst::MessagePtr& msg);

    QGst::PipelinePtr fPipeline;
    struct SDL_RWops* fRwops;
    long fDataLen;
    class RwopsApplicationSource* fAppSrc;
    bool fLooping;
};


#endif
