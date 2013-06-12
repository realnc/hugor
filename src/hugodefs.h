#ifndef HUGODEFS_H
#define HUGODEFS_H

#include <QColor>

QColor hugoColorToQt( int color );
void initSoundEngine();
void closeSoundEngine();
void muteSound( bool mute );
void calcFontDimensions();

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
