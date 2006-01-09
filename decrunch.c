/* decrunch.c
 *
 * based on load.c from:
 * Extended Module Player
 *
 * Copyright (C) 1996-1999 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * CHANGES: (modified for uade by mld)
 * removed all xmp related code)
 * added "custom" labels of pp20 files
 * added support for external unrar decruncher
 * added support for the external XPK Lib for Unix (the xType usage *g*)
 *
 * TODO:
 * real builtin support for XPK lib for Unix
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "decrunch.h"
#include "ppdepack.h"
#include "unsqsh.h"
#include "mmcmp.h"
#include "s404_dec.h"


enum {
    BUILTIN_PP = 1,
    BUILTIN_SQSH,
    BUILTIN_MMCMP,
    BUILTIN_S404
};


size_t atomic_fread(void *dst, FILE *f, size_t count)
{
    uint8_t *p = (uint8_t *) dst;
    size_t left = count;

    while (left > 0) {
	ssize_t nread = fread(p, 1, left, f);
	if (nread <= 0) {
	    fprintf(stderr, "atomic_fread() failed: %s\n", strerror(errno));
	    return 0;
	}
	left -= nread;
	p += nread;
    }

    return count;
}


size_t atomic_fwrite(FILE *f, void *src, size_t count)
{
    uint8_t *p = (uint8_t *) src;
    size_t left = count;

    while (left > 0) {
	ssize_t nwrite = fwrite(p, 1, left, f);
	if (nwrite <= 0) {
	    fprintf(stderr, "atomic_fwrite() failed: %s\n", strerror(errno));
	    return 0;
	}
	left -= nwrite;
	p += nwrite;
    }

    return count;
}


int decrunch(const char *filename, FILE *out, int pretend)
{
    uint8_t b[16];
    int builtin, res;
    char *packer;
    size_t nbytes;
    FILE *in;
    char dstname[PATH_MAX] = "";
    uint8_t *buf = NULL;

    if (filename[0]) {
	if ((in = fopen(filename, "rb")) == NULL) {
	    fprintf(stderr, "Unknown file %s\n", filename);
	    goto error;
	}
    } else {
	in = stdin;
    }

    packer = NULL;
    builtin = 0;

    nbytes = fread(b, 1, sizeof b, in);
    if (nbytes < sizeof b)
	goto error;

    if ((b[0] == 'P' && b[1] == 'X' && b[2] == '2' && b[3] == '0') ||
	(b[0] == 'P'  && b[1] == 'P'  && b[2] == '2'  && b[3] == '0')) {
        packer = "PowerPacker data";
	builtin = BUILTIN_PP;
    } else if (nbytes >= 12 && b[0] == 'X' && b[1] == 'P' && b[2] == 'K' && b[3] == 'F' &&
	b[8] == 'S' && b[9] == 'Q' && b[10] == 'S' && b[11] == 'H') {
	packer = "XPK SQSH";
	builtin = BUILTIN_SQSH;
    } else if (nbytes >= 8 && b[0] == 'z' && b[1] == 'i' && b[2] == 'R' && b[3] == 'C' &&
	b[4] == 'O' && b[5] == 'N' && b[6] == 'i' && b[7] == 'a') {
	packer = "MMCMP";
	builtin = BUILTIN_MMCMP;
    } else if (nbytes >= 16 && b[0] == 'S' && b[1] == '4' && b[2] == '0' && b[3] == '4' && b[4] < 0x80 && b[8] < 0x80 && b[12] < 0x80) {
	packer ="S404";
	builtin = BUILTIN_S404;
    }

    if (!packer)
	goto error;

    if (filename[0])
	fprintf(stderr, "File %s is in %s format.\n", filename, packer);
    else
	fprintf(stderr, "Stream is in %s format.\n", packer);

    if (pretend)
      return 0;

    if (in == stdin) {
	size_t bsize = 4096;

	if ((buf = malloc(bsize)) == NULL)
	    goto error;

	memcpy(buf, b, nbytes);

	while (1) {
	    size_t n;
	    if (nbytes == bsize) {
		uint8_t *newbuf;
		bsize *= 2;
		if ((newbuf = realloc(buf, bsize)) == NULL)
		    goto error;
		buf = newbuf;
	    }
	    n = fread(&buf[nbytes], 1, bsize - nbytes, in);
	    if (n <= 0)
		break;
	    nbytes += n;
	}

    } else {
	fseek(in, 0, SEEK_END);
	nbytes = ftell(in);
	fseek(in, 0, SEEK_SET);
	if ((buf = malloc(nbytes)) == NULL)
	    goto error;
	if (atomic_fread(buf, in, nbytes) == 0)
	    goto error;
    }
    
    if (nbytes <= sizeof b)
      goto error;

    if (out != stdout) {
	int fd;
	snprintf(dstname, sizeof dstname, "%s.XXXXXX", filename);
	if ((fd = mkstemp(dstname)) < 0) {
	    fprintf(stderr, "Could not create a temporary file: %s (%s)\n", dstname, strerror(errno));
	    goto error;
	}
	if ((out = fdopen(fd, "w")) == NULL) {
	    fprintf(stderr, "Could not fdopen temporary file: %s\n", dstname);
	    goto error;
	}
    }

    res = 0;
    switch (builtin) {
    case BUILTIN_PP:    
	res = decrunch_pp (buf, nbytes, out);
	break;
    case BUILTIN_SQSH:    
	res = decrunch_sqsh (buf, nbytes, out);
	break;
    case BUILTIN_MMCMP:    
	res = decrunch_mmcmp (buf, nbytes, out);
	break;
    case BUILTIN_S404:
	res = decrunch_s404(buf, nbytes, out);
	break;
    }

    free(buf);
    buf = NULL;

    if (res < 0)
      goto error;

    fclose(in);
    if (out != stdout && out != NULL && dstname[0]) {
	fclose(out);
	if (rename(dstname, filename)) {
	    fprintf(stderr, "Rename error: %s -> %s (%s)\n", dstname, filename, strerror(errno));
	    unlink(dstname);
	}
    }
    return 0;

 error:
    if (in)
	fclose(in);
    if (out != stdout && out != NULL)
      fclose(out);
    if (dstname[0])
	unlink(dstname);
    if (buf)
	free(buf);
    return -1;
}
