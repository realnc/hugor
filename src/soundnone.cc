// This is copyrighted software. More information is at the end of this file.
#include "hugodefs.h"
#include "hugohandlers.h"

#include <cstdio>

#include "hugorfile.h"

void initSoundEngine()
{}

void closeSoundEngine()
{}

void muteSound(bool /*mute*/)
{}

void updateMusicVolume()
{}

void updateSoundVolume()
{}

void updateSynthGain()
{}

bool isMusicPlaying()
{
    return false;
}

bool isSamplePlaying()
{
    return false;
}

void HugoHandlers::playmusic(HUGO_FILE /*infile*/, long /*reslength*/, char /*loop_flag*/,
                             int* result)
{
    *result = false;
}

void HugoHandlers::musicvolume(int /*vol*/)
{}

void HugoHandlers::stopmusic()
{}

void HugoHandlers::playsample(HUGO_FILE /*infile*/, long /*reslength*/, char /*loop_flag*/,
                              int* result)
{
    *result = false;
}

void HugoHandlers::samplevolume(int /*vol*/)
{}

void HugoHandlers::stopsample()
{}

/* Copyright (C) 2011-2019 Nikos Chantziaras
 *
 * This file is part of Hugor.
 *
 * Hugor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Hugor is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Hugor.  If not, see <http://www.gnu.org/licenses/>.
 */
