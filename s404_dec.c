/*
   StoneCracker S404 algorithm data decompression routine
   (c) 2006 Jouni 'Mr.Spiv' Korhonen. The code is in public domain.
  
   from shd:
   Some portability notes. We are using int32_t as a file size, and that fits
   all Amiga file sizes. size_t is of course the right choice.

   Warning: Code is not re-entrant.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include <assert.h>

#include "s404_dec.h"
#include "decrunch.h"


/* bit buffer for rolling data bit by bit from the compressed file */
static uint32_t _bb;
/* bits left in the bit buffer */
static int _bl;
/* bit buffer */
static uint8_t *_src;
static uint8_t *_org_src;


static int initGetb(uint8_t *src, uint32_t src_length)
{
  int eff;
  _src = src + src_length;
  _org_src = src;

  _bl = ntohs(* (uint16_t *) (_src));
  if (_bl & (~0xf))
    fprintf(stderr, "Warning: odd stonecracker data.\n");
  /* mask off any corrupt bits */
  _bl &= 0x000f;
  _src -= 2;    /* bit counter */

  _bb = ntohs(* (uint16_t *) (_src));
  _src -= 2;    /* last short  */

  eff = ntohs(* (uint16_t *) (_src));
  _src -= 2;    /* efficiency */

  return eff;
}


/* get nbits from the compressed stream */
static uint16_t getb(int nbits)
{
  _bb &= 0x0000ffff;

  if (_bl < nbits) {
    _bb <<= _bl;

    assert((intptr_t) _src >= (intptr_t) _org_src);

    _bb |= (_src[0] << 8);
    _bb |= _src[1];
    _src -= 2;

    nbits -= _bl;
    _bl = 16;
  }

  _bl -= nbits;
  _bb <<= nbits;
  return (_bb >> 16);
}


/* Returns bytes still to read.. or < 0 if error. */
int checkS404File(uint8_t *buf, size_t len,
		  int32_t *oLen, int32_t *pLen, int32_t *sLen )
{
  if (len < 16)
    return -1;

  if (memcmp(buf, "S404", 4) != 0)
    return -1;

  *sLen = ntohl(* (uint32_t *) &buf[4]);
  if (*sLen < 0)
    return -1;
  *oLen = ntohl(* (uint32_t *) &buf[8]); /* Depacked length */
  if (*oLen < 0)
    return -1;
  *pLen = ntohl(* (uint32_t *) &buf[12]); /* Packed length */
  if (*pLen < 0)
    return -1;

  return 0;
}


void decompressS404(uint8_t *src, uint8_t *orgdst,
		    int32_t dst_length, int32_t src_length)
{
  uint16_t w;
  int32_t eff;
  int32_t n;
  uint8_t *dst;
  int32_t oLen = dst_length;

  dst = orgdst + oLen;

  eff = initGetb(src, src_length);

  /*printf("_bl: %02X, _bb: %04X, eff: %d\n",_bl,_bb, eff);*/

  while (oLen > 0) {
    w = getb(9);

    /*printf("oLen: %d _bl: %02X, _bb: %04X, w: %04X\n",oLen,_bl,_bb,w);*/

    if (w < 0x100) {
      assert((intptr_t) dst > (intptr_t) orgdst);
      *--dst = w;
      /*printf("0+[8] -> %02X\n",w);*/
      oLen--;
    } else if (w == 0x13e || w == 0x13f) {
      w <<= 4;
      w |= getb(4);

      n = (w & 0x1f) + 14;
      oLen -= n;
      while (n-- > 0) {
        w = getb(8);

        /*printf("1+001+1111+[4] -> [8] -> %02X\n",w);*/
	assert((intptr_t) dst > (intptr_t) orgdst);
        *--dst = w;
      }
    } else {
      if (w >= 0x180) {
        /* copy 2-3 */
        n = w & 0x40 ? 3 : 2;
        
        if (w & 0x20) {
          /* dist 545 -> */
          w = (w & 0x1f) << (eff - 5);
          w |= getb(eff - 5);
          w += 544;
          /* printf("1+1+[1]+1+[%d] -> ", eff); */
        } else if (w & 0x30) {
          // dist 1 -> 32
          w = (w & 0x0f) << 1;
          w |= getb(1);
          /* printf("1+1+[1]+01+[5] %d %02X %d %04X-> ",n,w, _bl, _bb); */
        } else {
          /* dist 33 -> 544 */
          w = (w & 0x0f) << 5;
          w |= getb(5);
          w += 32;
          /* printf("1+1+[1]+00+[9] -> "); */
        }
      } else if (w >= 0x140) {
        /* copy 4-7 */
        n = ((w & 0x30) >> 4) + 4;
        
        if (w & 0x08) {
          /* dist 545 -> */
          w = (w & 0x07) << (eff - 3);
          w |= getb(eff - 3);
          w += 544;
          /* printf("1+01+[2]+1+[%d] -> ", eff); */
        } else if (w & 0x0c) {
          /* dist 1 -> 32 */
          w = (w & 0x03) << 3;
          w |= getb(3);
          /* printf("1+01+[2]+01+[5] -> "); */
        } else {
          /* dist 33 -> 544 */
          w = (w & 0x03) << 7;
          w |= getb(7);
          w += 32;
          /* printf("1+01+[2]+00+[9] -> "); */
        }
      } else if (w >= 0x120) {
        /* copy 8-22 */
        n = ((w & 0x1e) >> 1) + 8;
        
        if (w & 0x01) {
          /* dist 545 -> */
          w = getb(eff);
          w += 544;
          /* printf("1+001+[4]+1+[%d] -> ", eff); */
        } else {
          w = getb(6);

          if (w & 0x20) {
            /* dist 1 -> 32 */
            w &= 0x1f;
            /* printf("1+001+[4]+001+[5] -> "); */
          } else {
            /* dist 33 -> 544 */
            w <<= 4;
            w |= getb(4);

            w += 32;
            /* printf("1+001+[4]+00+[9] -> "); */
          }
        }
      } else {
        w = (w & 0x1f) << 3;
	w |= getb(3);
        n = 23;

        while (w == 0xff) {
          n += w;
          w = getb(8);
        }
        n += w;

        w = getb(7);

        if (w & 0x40) {
          /* dist 545 -> */
          w = (w & 0x3f) << (eff - 6);
          w |= getb(eff - 6);

          w += 544;
        } else if (w & 0x20) {
          /* dist 1 -> 32 */
          w &= 0x1f;
          /* printf("1+000+[8]+01+[5] -> "); */
        } else {
          /* dist 33 -> 544; */
          w <<= 4;
	  w |= getb(4);

          w += 32;
          /* printf("1+000+[8]+00+[9] -> "); */
        }
      }

      /* printf("<%d,%d>\n",n,w+1); fflush(stdout); */
      oLen -= n;

      while (n-- > 0) {
        /* printf("Copying: %02X\n",dst[w]); */
	dst--;
	assert((intptr_t) dst >= (intptr_t) orgdst);
	assert((intptr_t) (dst + w + 1) < (intptr_t) (orgdst + dst_length));
	*dst = dst[w + 1];
      }
    }
  }
}


int decrunch_s404(uint8_t *src, size_t s, FILE *out)
{
  int32_t oLen, sLen, pLen;
  uint8_t *dst = NULL;

  if (checkS404File(src, s, &oLen, &pLen, &sLen)) {
    fprintf(stderr,"S404 Error: checkS404File() failed..\n");
    goto error;
  }

  /* fprintf(stderr, "\tOriginal length: %d\n", oLen);
     fprintf(stderr, "\tCompressed length: %d\n", pLen);
     fprintf(stderr, "\tSecurity length: %d\n", sLen);
  */

  if ((dst = malloc(oLen)) == NULL) {
    fprintf(stderr,"S404 Error: malloc(%d) failed..\n", oLen);
    goto error;
  }

  /* src + 16 skips S404 header */
  decompressS404(src + 16, dst, oLen, pLen);

  if (atomic_fwrite(out, dst, oLen) == 0) {
      fprintf(stderr,"S404 Error: fwrite() failed..\n");
      goto error;
  }

  free(dst);
  return 0;

 error:
  free(dst);
  return -1;
}
