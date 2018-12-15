/* Copyright 2015 Nikos Chantziaras
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
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with the Hugo Engine (or a modified version of the Hugo Engine), containing
 * parts covered by the terms of the Hugo License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 * Corresponding Source for a non-source form of such a combination shall
 * include the source code for the parts of the Hugo Engine used as well as
 * that of the covered work.
 */
#ifndef HUGODEFS_H
#define HUGODEFS_H

class QColor;

QColor hugoColorToQt( int color );
void initSoundEngine();
void initVideoEngine( int& argc, char* argv[] );
void closeSoundEngine();
void closeVideoEngine();
void muteSound( bool mute );
void muteVideo( bool mute );
void updateMusicVolume();
bool isMusicPlaying();
bool isSamplePlaying();
void updateSoundVolume();
void updateSynthGain();
void updateVideoVolume();

// Defined Hugo colors.
#define HUGO_BLACK         0
#define HUGO_BLUE          1
#define HUGO_GREEN         2
#define HUGO_CYAN          3
#define HUGO_RED           4
#define HUGO_MAGENTA       5
#define HUGO_BROWN         6
#define HUGO_WHITE         7
#define HUGO_DARK_GRAY     8
#define HUGO_LIGHT_BLUE    9
#define HUGO_LIGHT_GREEN   10
#define HUGO_LIGHT_CYAN    11
#define HUGO_LIGHT_RED     12
#define HUGO_LIGHT_MAGENTA 13
#define HUGO_YELLOW        14
#define HUGO_BRIGHT_WHITE  15


#endif // HUGODEFS_H
