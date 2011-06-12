#ifdef SOUND_FMOD
#   include <fmod.hpp>
#   include <fmod_errors.h>
#endif

#include <QDebug>
#include <QFile>
#include <QFileInfo>

extern "C" {
#include "heheader.h"
}
#include "happlication.h"
#include "settings.h"
#include "hugodefs.h"


// Global FMOD system object.
FMOD::System* fmSystem = 0;

// Channel of currently playing music/sample.
static FMOD::Channel* musicChan = 0;
static FMOD::Channel* sampleChan = 0;

// Current music/sample volumes.
static float musicVol = 1.0f;
static float sampleVol = 1.0f;

// Global mute flag.
static bool isMuted = false;


void initSoundEngine()
{
    FMOD_RESULT res = FMOD::System_Create(&fmSystem);
    if (res != FMOD_OK) {
        qWarning("Unable to initialize FMOD sound system: %s", FMOD_ErrorString(res));
        exit(1);
    }
    res = fmSystem->init(4, FMOD_INIT_NORMAL, 0);
    if (res != FMOD_OK) {
        qWarning("Unable to initialize FMOD sound system: %s", FMOD_ErrorString(res));
        exit(1);
    }
}


void closeSoundEngine()
{
    fmSystem->release();
}


static bool
loadAndPlaySound( HUGO_FILE infile, long reslength, char loop_flag,
                  FMOD::Sound*& sound, FMOD_CREATESOUNDEXINFO& info,
                  FMOD::Channel*& channel, float volume )
{
    // If an FMOD::Sound* already exists, delete it first. This will
    // also stop the music if it's currently playing.
    if (sound != 0) {
        sound->release();
        sound = 0;
    }

    info.fileoffset = ftell(infile);
    info.length = static_cast<unsigned>(reslength);
    // Done with the file.
    fclose(infile);
    FMOD_RESULT res;
    res = fmSystem->createStream(loaded_filename,
                                 FMOD_LOOP_NORMAL | FMOD_2D | FMOD_HARDWARE,
                                 &info, &sound);
    if (res != FMOD_OK) {
        qWarning() << "createStream:" << FMOD_ErrorString(res);
        return false;
    }

    // Start playing the music. Loop forever if 'loop_flag' is true.
    // Otherwise, just play it once.
    sound->setLoopCount(loop_flag ? -1 : 0);
    res = fmSystem->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
    if (res != FMOD_OK) {
        qWarning() << "ERROR:" << FMOD_ErrorString(res);
        channel = 0;
        return false;
    }
    // Adjust volume and mute flag.
    channel->setVolume(volume);
    channel->setMute(isMuted);
    return true;
}


extern "C" int
hugo_playmusic( HUGO_FILE infile, long reslength, char loop_flag )
{
    if (not hApp->settings()->enableMusic) {
        fclose(infile);
        return false;
    }

    // We only play one music track at a time, so it's enough
    // to make these static.
    static FMOD::Sound* music = 0;
    static FMOD_CREATESOUNDEXINFO info;
    info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

    return loadAndPlaySound(infile, reslength, loop_flag, music,
                            info, musicChan, musicVol);
}


extern "C" void
hugo_musicvolume( int vol )
{
    if (vol < 0)
        vol = 0;
    else if (vol > 100)
        vol = 100;

    musicVol = vol / 100.0f;
}


extern "C" void
hugo_stopmusic( void )
{
    if (musicChan != 0) {
        musicChan->stop();
    }
}


void
muteSound( bool mute )
{
    if (musicChan != 0) {
        musicChan->setMute(mute);
    }
    if (sampleChan != 0) {
        sampleChan->setMute(mute);
    }
    isMuted = mute;
}


extern "C" int
hugo_playsample( HUGO_FILE infile, long reslength, char loop_flag )
{
    if (not hApp->settings()->enableSoundEffects) {
        fclose(infile);
        return false;
    }

    // We only play one sample at a time, so it's enough
    // to make these static.
    static FMOD::Sound* sample = 0;
    static FMOD_CREATESOUNDEXINFO info;
    info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

    return loadAndPlaySound(infile, reslength, loop_flag, sample,
                            info, sampleChan, sampleVol);
}


extern "C" void
hugo_samplevolume( int vol )
{
    if (vol < 0)
        vol = 0;
    else if (vol > 100)
        vol = 100;

    sampleVol = vol / 100.0f;
}


extern "C" void
hugo_stopsample( void )
{
    if (sampleChan != 0) {
        sampleChan->stop();
    }
}
