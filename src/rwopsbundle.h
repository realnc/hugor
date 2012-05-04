#ifndef RWOPSBUNDLE_H
#define RWOPSBUNDLE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create a custom RWops for the given file and media resource length.
 * The file must have already been positioned to the beginning of the
 * resource.
 *
 * The RWops takes ownership of the file; when the RWops is deleted or
 * closed, fclose() will be called on 'mediaBundle'.
 *
 * In case or errors, NULL is returned and 'mediaBundle' is not closed.
 * SDL_GetError() can be used to obtain an error string.
 */
SDL_RWops*
RWFromMediaBundle( FILE* mediaBundle, long resLength );

#ifdef __cplusplus
}
#endif

#endif
