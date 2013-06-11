#ifndef RWOPSQIODEV_H
#define RWOPSQIODEV_H

#include <QIODevice>


class RwopsQIODevice: public QIODevice {
    Q_OBJECT

public:
    explicit RwopsQIODevice(QObject* parent)
        : QIODevice(parent),
          fRwops(0),
          fSize(0)
    { }

    bool atEnd();
    bool isSequential();
    bool seek(qint64 pos);
    qint64 size() const;
    void close();

    bool open(struct SDL_RWops* rwops, OpenMode mode);

protected:
    qint64 readData(char* data, qint64 len);
    qint64 writeData(const char*, qint64);

private:
    struct SDL_RWops* fRwops;
    long fSize;
};


#endif
