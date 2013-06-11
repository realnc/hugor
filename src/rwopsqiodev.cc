#include "rwopsqiodev.h"

#include <SDL_rwops.h>


bool
RwopsQIODevice::atEnd()
{
    return fRwops and (fSize == SDL_RWtell(fRwops));
}


bool
RwopsQIODevice::isSequential()
{
    return fRwops and fRwops->seek;
}


bool RwopsQIODevice::seek(qint64 pos)
{
    if (not fRwops) {
        return false;
    }
    QIODevice::seek(pos);
    long newPos = SDL_RWseek(fRwops, pos, RW_SEEK_SET);
    if (newPos != pos) {
        return false;
    }
    return true;
}


qint64
RwopsQIODevice::size() const
{
    if (fRwops) {
        return fSize;
    }
    return 0;
}


void
RwopsQIODevice::close()
{
    fRwops = 0;
    fSize = 0;
    QIODevice::close();
}


bool
RwopsQIODevice::open(SDL_RWops* rwops, QIODevice::OpenMode mode)
{
    if (not rwops) {
        return false;
    }
    long pos = SDL_RWtell(rwops);
    long siz = SDL_RWseek(rwops, 0, RW_SEEK_END) - SDL_RWseek(rwops, 0, RW_SEEK_SET);
    SDL_RWseek(rwops, pos, RW_SEEK_SET);
    fSize = siz;
    fRwops = rwops;
    return QIODevice::open(mode);
}


qint64
RwopsQIODevice::readData(char* data, qint64 len)
{
    if (not fRwops) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }
    long cnt = SDL_RWread(fRwops, data, 1, len);
    if (cnt == 0) {
        return -1;
    }
    return cnt;
}


qint64
RwopsQIODevice::writeData(const char*, qint64)
{
    return -1;
}
