#include <QThread>
#include "enginerunner.h"
extern "C" {
#include "heheader.h"
}


void
EngineRunner::runEngine()
{
    char argv0[] = "hugor";
    char* argv1 = new char[fGameFile.toLocal8Bit().size() + 1];
    strcpy(argv1, fGameFile.toLocal8Bit().constData());
    char* argv[2] = {argv0, argv1};
    he_main(2, argv);
    emit finished();
}
