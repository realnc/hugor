#include <cstdio>
#include "hugohandlers.h"
#include "hugodefs.h"


void
initSoundEngine()
{ }


void
closeSoundEngine()
{ }


void
muteSound(bool)
{ }


void
updateMusicVolume()
{ }


void
updateSoundVolume()
{ }


void
HugoHandlers::playmusic(HUGO_FILE infile, long, char, int* result)
{
    std::fclose(infile);
    *result = false;
}


void
HugoHandlers::musicvolume(int)
{ }


void
HugoHandlers::stopmusic()
{ }


void
HugoHandlers::playsample(HUGO_FILE infile, long, char, int* result)
{
    std::fclose(infile);
    *result = false;
}


void
HugoHandlers::samplevolume(int)
{ }


void
HugoHandlers::stopsample()
{ }
