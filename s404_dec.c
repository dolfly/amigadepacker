//
// StoneCracker S404 algorithm data decompression routine
// (c) 2006 Jouni 'Mr.Spiv' Korhonen
//
// v0.1
//
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
//
//

static int _bc;
static unsigned long _bb;

inline int initGetb( unsigned char **b ) { 
  int eff;
  _bc = (*b)[0]; _bc <<= 8; _bc |= (*b)[1]; *b -= 2;    // bit counter
  _bb = (*b)[0]; _bb <<= 8; _bb |= (*b)[1]; *b -= 2;    // last short 
  eff = (*b)[0]; eff <<= 8; eff |= (*b)[1]; *b -= 2;    // efficiency

  // check for _bc validity as it may be bugged
  _bc &= 0x000f;

  return eff;
}

inline unsigned short getb( unsigned char **b, int l ) {
  int m = l;
  _bb &= 0x0000ffff;

  if (_bc < l) {
    _bb <<= _bc;
    _bb |= ((*b)[0] << 8); _bb |= (*b)[1]; *b -= 2;
    l -= _bc;
    _bc = 16;
  }
  _bc -= l;
  _bb <<= l;
  return (_bb >> 16);
}

//
//
// Returns bytes still to read.. or < 0 if error.
//

long checkS404File( FILE *fh, long *oLen, long *pLen, long *sLen ) {
  unsigned char buf[16];
  long tmp;
  long len;

  fseek(fh,0,SEEK_END); len = ftell(fh); fseek(fh,0,SEEK_SET);

  if (fread((char *)&buf[0], 1 , 16, fh) != 16) { return -1; }
  if (strncmp((char *)&buf[0],"S404",4)) { return -2; }

  tmp  = buf[4]; tmp <<= 8; tmp |= buf[5]; tmp <<= 8;
  tmp |= buf[6]; tmp <<= 8; tmp |= buf[7];
  *sLen = tmp;

  tmp  = buf[8]; tmp <<= 8; tmp |= buf[9]; tmp <<= 8;
  tmp |= buf[10]; tmp <<= 8; tmp |= buf[11];
  *oLen = tmp;

  tmp  = buf[12]; tmp <<= 8; tmp |= buf[13]; tmp <<= 8;
  tmp |= buf[14]; tmp <<= 8; tmp |= buf[15];
  *pLen = tmp;

  return len-16;
}

int decompressS404( unsigned char *src, unsigned char *dst, long oLen, long pLen ) {
  unsigned short w;
  int eff;
  int n;

  src += pLen;
  dst += oLen;
  eff = initGetb(&src);

  //printf("_bc: %02X, _bb: %04X, eff: %d\n",_bc,_bb,eff);

  while (oLen > 0) {
    //{
    w = getb(&src,9);
    //printf("oLen: %d _bc: %02X, _bb: %04X, w: %04X\n",oLen,_bc,_bb,w);

    if (w < 0x100) {
      *--dst = w;
      //printf("0+[8] -> %02X\n",w);
      oLen--;
    } else if (w == 0x13e || w == 0x13f) {
      w <<= 4;
      w |= getb(&src,4);
      n = (w & 0x1f) + 14;
      oLen -= n;
      while (n-- > 0) {
        w = getb(&src,8);
        //printf("1+001+1111+[4] -> [8] -> %02X\n",w);
        *--dst = w;
      }
    } else {
      if (w >= 0x180) {
        // copy 2-3
        n = w & 0x40 ? 3 : 2;
        
        if (w & 0x20) {
          // dist 545 ->
          w = (w & 0x1f) << (eff-5);
          w |= getb(&src,eff-5);
          w += 544;
          //printf("1+1+[1]+1+[%d] -> ",eff);
        } else if (w & 0x30) {
          // dist 1 -> 32
          w = (w & 0x0f) << 1;
          w |= getb(&src,1);
          //printf("1+1+[1]+01+[5] %d %02X %d %04X-> ",n,w, _bc, _bb);
        } else {
          // dist 33 -> 544
          w = (w & 0x0f) << 5;
          w |= getb(&src,5);
          w += 32;
          //printf("1+1+[1]+00+[9] -> ");
        }
      } else if (w >= 0x140) {
        // copy 4-7
        n = ((w & 0x30) >> 4) + 4;
        
        if (w & 0x08) {
          // dist 545 ->
          w = (w & 0x07) << (eff-3);
          w |= getb(&src,eff-3);
          w += 544;
          //printf("1+01+[2]+1+[%d] -> ",eff);
        } else if (w & 0x0c) {
          // dist 1 -> 32
          w = (w & 0x03) << 3;
          w |= getb(&src,3);
          //printf("1+01+[2]+01+[5] -> ");
        } else {
          // dist 33 -> 544
          w = (w & 0x03) << 7;
          w |= getb(&src,7);
          w += 32;
          //printf("1+01+[2]+00+[9] -> ");
        }
      } else if (w >= 0x120) {
        // copy 8-22
        n = ((w & 0x1e) >> 1) + 8;
        
        if (w & 0x01) {
          // dist 545 ->
          w = getb(&src,eff);
          w += 544;
          //printf("1+001+[4]+1+[%d] -> ",eff);
        } else {
          w = getb(&src,6);
          
          if (w & 0x20) {
            // dist 1 -> 32
            w &= 0x1f;
            //printf("1+001+[4]+001+[5] -> ");
          } else {
            // dist 33 -> 544
            w <<= 4;
            w |= getb(&src,4);
            w += 32;
            //printf("1+001+[4]+00+[9] -> ");
          }
        }
      } else {
        w = (w & 0x1f) << 3; w |= getb(&src,3);
        n = 23;

        while (w == 0xff) {
          n += w;
          w = getb(&src,8);
        }
        n += w;

        w = getb(&src,7);

        if (w & 0x40) {
          // dist 545 ->
          w = (w & 0x3f) << (eff - 6);
          w |= getb(&src,eff - 6);
          w += 544;
        } else if (w & 0x20) {
          // dist 1 -> 32
          w &= 0x1f;
          //printf("1+000+[8]+01+[5] -> ");
        } else {
          // dist 33 -> 544;
          w <<= 4; w |= getb(&src,4);
          w += 32;
          //printf("1+000+[8]+00+[9] -> ");
        }
      }

      //printf("<%d,%d>\n",n,w+1); fflush(stdout);
      oLen -= n;

      while (n-- > 0) {
        //printf("Copying: %02X\n",dst[w]);
        *--dst = dst[w+1];
      }
    }
  }
  return 0;
}




int main( int argc, char **argv ) {
  long oLen, sLen, pLen;
  unsigned char *src;
  unsigned char *dst;
  int n;
  FILE *fh;

  if (argc != 3) {
    fprintf(stderr,"Usage: %s infile outfile\n",argv[1]);
    exit(EXIT_FAILURE);
  }
  if ((fh = fopen(argv[1],"r")) == NULL) {
    fprintf(stderr,"** Error: fopen(%s) failed..\n",argv[1]);
    exit(EXIT_FAILURE);
  }
  if ((n = checkS404File(fh,&oLen,&pLen,&sLen)) < 0) {
    fprintf(stderr,"** Error: checkS404File() failed..\n");
    fclose(fh);
    exit(EXIT_FAILURE);
  }

  printf("%s is a S404 compressed file\n",argv[1]);
  printf("\tOriginal length: %d\n",oLen);
  printf("\tCompressed length: %d\n",pLen);
  printf("\tSecurity length: %d\n",sLen);
  printf("\tRead length: %d\n",n);

  if ((src = malloc(n)) == NULL) {
    fprintf(stderr,"** Error: malloc(%d) failed..\n",n);
    fclose(fh);
    exit(EXIT_FAILURE);
  }
  if ((dst = malloc(oLen)) == NULL) {
    fprintf(stderr,"** Error: malloc(%d) failed..\n",oLen);
    free(src);
    fclose(fh);
    exit(EXIT_FAILURE);
  }
  if (fread(src,1,n,fh) != n) {
    fprintf(stderr,"** Error: fread() failed..\n");
    free(src);
    free(dst);
    fclose(fh);
    exit(EXIT_FAILURE);
  }

  fclose(fh);

  if ((fh = fopen(argv[2],"w")) == NULL) {
    fprintf(stderr,"** Error: fopen(%s) failed..\n",argv[2]);
    free(src);
    free(dst);
    exit(EXIT_FAILURE);
  }

  decompressS404(src,dst,oLen,pLen);

  if (fwrite(dst,1,oLen,fh) != oLen) {
    fprintf(stderr,"** Error: fwrite() failed..\n");
    free(src);
    free(dst);
    fclose(fh);
    exit(EXIT_FAILURE);
  }

  printf("Done...\n");

  free(src);
  free(dst);
  fclose(fh);

  return 0;
}
