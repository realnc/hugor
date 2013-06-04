#ifndef RWOPSAPPSRC
#define RWOPSAPPSRC

#include <QGst/Utils/ApplicationSource>


/* Custom appsrc that reads video data from an SDL_RWops. The RWops itself
 * is managed externally; we will neither close nor free it at any point.
 */
class RwopsApplicationSource: public QGst::Utils::ApplicationSource {
public:
    explicit RwopsApplicationSource(struct SDL_RWops* rwops)
        : fRwops(rwops)
    { }

    void setSource(SDL_RWops* rwops);

protected:
    virtual void needData(uint len);
    virtual bool seekData(quint64 offs);

private:
    SDL_RWops* fRwops;
};


#endif
