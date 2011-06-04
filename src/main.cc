#include <SDL.h>
#include <SDL_mixer.h>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileDialog>

extern "C" {
#include "heheader.h"
}
#include "happlication.h"
#include "settings.h"

// On some platforms, SDL redefines main in order to provide a
// platform-specific main() implementation.  However, Qt handles this too,
// so things can get weird.  We need to make sure main is not redefined so
// that Qt can find our own implementation and SDL will not try to do
// platform-specific initialization work (like launching the Cocoa event-loop
// or setting up the application menu on OS X, or redirecting stdout and stderr
// to text files on Windows), which would break things.
#ifdef main
#  undef main
#endif

int main( int argc, char* argv[] )
{
    // Initialize only the audio part of SDL.
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        qWarning("Unable to initialize sound system: %s", SDL_GetError());
        return 1;
    }

    // This will preload the needed codecs now instead of constantly loading
    // and unloading them each time a sound is played/stopped.  This is only
    // available in SDL_Mixer 1.2.10 and newer.
#if (MIX_MAJOR_VERSION > 1) \
    || ((MIX_MAJOR_VERSION == 1) && (MIX_MINOR_VERSION > 2)) \
    || ((MIX_MAJOR_VERSION == 1) && (MIX_MINOR_VERSION == 2) && (MIX_PATCHLEVEL > 9))
    int sdlFormats = MIX_INIT_MP3 | MIX_INIT_MOD;
    if (Mix_Init((sdlFormats & sdlFormats) != sdlFormats)) {
        qWarning("Unable to load MP3 and/or MOD audio formats: %s", Mix_GetError());
        return 1;
    }
#endif

    // Initialize the mixer. 44.1kHz, default sample format,
    // 2 channels (stereo) and a 4k chunk size.
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) != 0) {
        qWarning("Unable to initialize audio mixer: %s", Mix_GetError());
        return 1;
    }
    Mix_AllocateChannels(8);

    HApplication* app = new HApplication(argc, argv, "HugoQt", "0.1",
                                         "Nikos Chantziaras", "HugoQt.net");
    // Filename of the game to run.
    QString gameFileName;

    const QStringList& args = app->arguments();
    if (args.size() == 2) {
        if (QFile::exists(args.at(1))) {
            gameFileName = args.at(1);
        } else if (QFile::exists(args.at(1) + QString::fromAscii(".hex"))) {
            gameFileName = args.at(1) + QString::fromAscii(".hex");
        } else {
            qWarning() << "File" << args.at(1) << "not found.";
        }
    }

    if (gameFileName.isEmpty() and app->settings()->askForGameFile) {
        gameFileName = QFileDialog::getOpenFileName(0, QObject::tr("Choose the story file you wish to play"),
                                                    QString::fromAscii(""),
                                                    QObject::tr("Hugo Games")
                                                    + QString::fromAscii("(*.hex *.Hex *.HEX)"));
    }

    QMetaObject::invokeMethod(app, "main", Qt::QueuedConnection, Q_ARG(QString, gameFileName));
    int ret = app->exec();
    delete app;

    // Shut down SDL and SDL_mixer.
    Mix_ChannelFinished(0);
    Mix_HookMusicFinished(0);
    // Close the audio device as many times as it was opened.
    int opened = Mix_QuerySpec(0, 0, 0);
    for (int i = 0; i < opened; ++i) {
        Mix_CloseAudio();
    }
    SDL_Quit();
    return ret;
}
