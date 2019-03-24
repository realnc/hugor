// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create a custom RWops for the given file and media resource length. The file must have already
 * been positioned to the beginning of the resource.
 *
 * The RWops takes ownership of the file; when the RWops is deleted or closed, fclose() will be
 * called on 'mediaBundle'.
 *
 * In case or errors, NULL is returned and 'mediaBundle' is not closed. SDL_GetError() can be used
 * to obtain an error string.
 */
struct SDL_RWops* RWFromMediaBundle(FILE* mediaBundle, long resLength);

#ifdef __cplusplus
}
#endif

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
