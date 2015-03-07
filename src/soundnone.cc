extern "C" {
#include "heheader.h"
}
#include "hugodefs.h"


void
initSoundEngine()
{ }


void
closeSoundEngine()
{ }


int
hugo_playmusic( HUGO_FILE infile, long, char )
{
    fclose(infile);
    return false;
}


void
hugo_musicvolume( int )
{ }


void
hugo_stopmusic( void )
{ }


void
muteSound( bool )
{ }


int
hugo_playsample( HUGO_FILE infile, long, char )
{
    fclose(infile);
    return false;
}


void
hugo_samplevolume( int )
{ }


void
hugo_stopsample( void )
{ }
