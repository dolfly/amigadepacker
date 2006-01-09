/* Based on the decr. code of:
 *
 * Extended Module Player
 * Copyright (C) 1996-1999 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * modified for uade by mld
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#ifndef _DECRUNCH_H_
#define _DECRUNCH_H_

#include <stdio.h>

size_t atomic_fread(void *dst, FILE *f, size_t count);
size_t atomic_fwrite(FILE *f, void *src, size_t count);

int decrunch (const char *filename, FILE *out, int pretend);

#endif
