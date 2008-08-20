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
#include <assert.h>

#include "decrunch.h"
#include "ppdepack.h"
#include "unsqsh.h"
#include "mmcmp.h"
#include "s404_dec.h"
#include "compat.h"


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


static int read_packed_data_to_memory(uint8_t **buf, size_t *nbytes,
				      uint8_t *header, FILE *in)
{
    if (in == stdin) {
	size_t bsize = 4096;

	*buf = malloc(bsize);
	if (*buf == NULL)
	    return -1;

        /* header[] contains nbytes of data initially */
	memcpy(*buf, header, *nbytes);

	while (1) {
	    size_t n;

	    if (*nbytes == bsize) {
		uint8_t *newbuf;

		bsize *= 2;

		newbuf = realloc(*buf, bsize);
		if (newbuf == NULL)
		    return -1;

		*buf = newbuf;
	    }

	    n = fread(&(*buf)[*nbytes], 1, bsize - *nbytes, in);

	    if (n <= 0)
		break;

	    *nbytes += n;
	}

    } else {

	fseek(in, 0, SEEK_END);
	*nbytes = ftell(in);
	fseek(in, 0, SEEK_SET);

	*buf = malloc(*nbytes);
	if (*buf == NULL)
	    return -1;

	if (atomic_fread(*buf, in, *nbytes) == 0)
	    return -1;
    }

    return 0;
}


static struct decruncher *check_header(uint8_t b[16])
{
    struct decruncher *decruncher = NULL;

    if ((b[0] == 'P' && b[1] == 'X' && b[2] == '2' && b[3] == '0') ||
	(b[0] == 'P' && b[1] == 'P' && b[2] == '2' && b[3] == '0')) {
	decruncher = &decruncher_pp;

    } else if (b[0] == 'X' && b[1] == 'P' && b[2] == 'K' && b[3] == 'F' &&
	       b[8] == 'S' && b[9] == 'Q' && b[10] == 'S' && b[11] == 'H') {
	decruncher = &decruncher_sqsh;

    } else if (b[0] == 'z' && b[1] == 'i' && b[2] == 'R' && b[3] == 'C' &&
	       b[4] == 'O' && b[5] == 'N' && b[6] == 'i' && b[7] == 'a') {
	decruncher = &decruncher_mmcmp;

    } else if (b[0] == 'S' && b[1] == '4' && b[2] == '0' && b[3] == '4' &&
	       b[4] < 0x80 && b[8] < 0x80 && b[12] < 0x80) {
	decruncher = &decruncher_s404;
    }

    return decruncher;
}


int decrunch(FILE *out, const char *filename, int pretend)
{
    uint8_t b[16];
    int res;
    size_t nbytes;
    FILE *in;
    char dstname[PATH_MAX] = "";
    uint8_t *buf = NULL;
    struct decruncher *decruncher;
    int output_to_same_file = (out == NULL);

    if (filename) {
	in = fopen(filename, "rb");
	if (in == NULL) {
	    fprintf(stderr, "Unknown file %s\n", filename);
	    goto error;
	}
    } else {
	in = stdin;
    }

    nbytes = fread(b, 1, sizeof b, in);
    if (nbytes < sizeof b)
	goto error;

    decruncher = check_header(b);
    if (decruncher == NULL)
	goto error;

    if (filename)
	fprintf(stderr, "File %s is in %s format.\n", filename, decruncher->name);
    else
	fprintf(stderr, "Stream is in %s format.\n", decruncher->name);

    if (pretend)
      return 0;

    if (read_packed_data_to_memory(&buf, &nbytes, b, in))
	goto error;

    if (nbytes <= sizeof b)
      goto error;

    if (output_to_same_file) {
	int fd;

	snprintf(dstname, sizeof dstname, "%s.XXXXXX", filename);

	fd = mkstemp(dstname);
	if (fd < 0) {
	    fprintf(stderr, "Could not create a temporary file: %s (%s)\n", dstname, strerror(errno));
	    goto error;
	}

	out = fdopen(fd, "wb");
	if (out == NULL) {
	    fprintf(stderr, "Could not fdopen temporary file: %s\n", dstname);
	    goto error;
	}
    }

    res = decruncher->decrunch(buf, nbytes, out);

    if (res < 0)
      goto error;

    free(buf);

    fclose(in);

    if (output_to_same_file) {
	fclose(out);
	out = NULL;

	assert(dstname[0] != 0);

#ifdef RENAME_WORKAROUND
	/* Don't check return value of unlink(). This is for MinGW. */
	unlink(filename);
#endif

	if (rename(dstname, filename)) {
	    fprintf(stderr, "Rename error: %s -> %s (%s)\n", dstname, filename, strerror(errno));
	    unlink(dstname);
	}
    }

    return 0;

 error:
    fclose(in);

    if (dstname[0])
	unlink(dstname);

    if (buf)
	free(buf);

    return -1;
}
