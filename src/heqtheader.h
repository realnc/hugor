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

#if __STDC_VERSION__ >= 199901L
/* We're using a C99 compiler, meaning 'inline' is supported. */
#define HUGO_INLINE static inline
#else
/* Most other compilers support __inline with mostly the same semantics. */
#define HUGO_INLINE static __inline
#endif

#define PRINTFATALERROR printFatalError
void printFatalError( char* a );
#define NO_TERMINAL_LINEFEED
#define FRONT_END
#define GRAPHICS_SUPPORTED
#define SOUND_SUPPORTED
#define USE_TEXTBUFFER
#define USE_SMARTFORMATTING
#define SCROLLBACK_DEFINED


#endif /* HEQTHEADER_H */
