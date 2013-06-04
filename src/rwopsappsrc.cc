#include "rwopsappsrc.h"

#include <SDL_rwops.h>


void
RwopsApplicationSource::setSource(SDL_RWops* rwops)
{
    // Make sure we don't get any more data requests on the old source.
    this->endOfStream();
    // Switch to the new source.
    this->fRwops = rwops;
}


void
RwopsApplicationSource::needData(uint len)
{
    QGst::BufferPtr buffer = QGst::Buffer::create(len);
    long cnt = SDL_RWread(fRwops, buffer->data(), 1, len);
    this->pushBuffer(buffer);
    // Indicate EOS if there's no more data to be had from the RWops.
    if (cnt < len) {
        this->endOfStream();
    }
}


bool
RwopsApplicationSource::seekData(quint64 offs)
{
    return SDL_RWseek(fRwops, offs, RW_SEEK_SET) == (int)offs;
}
