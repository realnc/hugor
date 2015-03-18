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


int
HugoHandlers::playmusic(HUGO_FILE infile, long, char)
{
    std::fclose(infile);
    return false;
}


void
HugoHandlers::musicvolume(int)
{ }


void
HugoHandlers::stopmusic()
{ }


int
HugoHandlers::playsample(HUGO_FILE infile, long, char)
{
    std::fclose(infile);
    return false;
}


void
HugoHandlers::samplevolume(int)
{ }


void
HugoHandlers::stopsample()
{ }
