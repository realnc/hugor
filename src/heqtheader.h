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
#ifndef HEQTHEADER_H
#define HEQTHEADER_H


#define PORTER_NAME "Nikos Chantziaras"
#define PORT_NAME "Hugor"
#define PROGRAM_NAME "hugor"

#define DEF_PRN ""

#define MAXPATH         256
#define MAXDRIVE        256
#define MAXDIR          256
#define MAXFILENAME     256
#define MAXEXT          256

#define DEF_FCOLOR      7
#define DEF_BGCOLOR     1
#define DEF_SLFCOLOR	15
#define DEF_SLBGCOLOR	0

#define MAXBUFFER 255
#define MAXUNDO 1024

#define HUGO_FILE FILE*

#if __STDC_VERSION__ >= 199901L
/* We're using a C99 compiler, meaning 'inline' is supported. */
#define HUGO_INLINE static inline
#else
/* Most other compilers support __inline with mostly the same semantics. */
#define HUGO_INLINE static __inline
#endif

#define PRINTFATALERROR printFatalError
#define NO_TERMINAL_LINEFEED
#define FRONT_END
#define GRAPHICS_SUPPORTED
#define SOUND_SUPPORTED
#define USE_TEXTBUFFER
#define USE_SMARTFORMATTING
#define SCROLLBACK_DEFINED
#define CUSTOM_SCRIPT_WRITE
#define HUGO_FCLOSE

#ifdef __cplusplus
extern "C" {
#endif
void printFatalError(char* a);
int hugo_displaypicture(HUGO_FILE infile, long len);
int hugo_playmusic(HUGO_FILE infile, long reslength, char loop_flag);
void hugo_musicvolume(int vol);
void hugo_stopmusic(void);
int hugo_playsample(HUGO_FILE infile, long reslength, char loop_flag);
void hugo_samplevolume(int vol);
void hugo_stopsample(void);
int hugo_playvideo(HUGO_FILE infile, long len, char loop, char bg, int vol);
void hugo_stopvideo(void);
int hugo_writetoscript(const char* s);
int hugo_fclose(HUGO_FILE file);
#ifdef __cplusplus
}
#endif


#endif /* HEQTHEADER_H */
