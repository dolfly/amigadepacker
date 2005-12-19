#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "version.h"
#include "decrunch.h"

int main(int argc, char **argv)
{
    int ret, res, i;
    int use_stdout = 0;
    int pretend = 0;

    struct option long_options[] = {
	{"help", 0, NULL, 'h'},
	{"pretend", 0, NULL, 'p'},
	{"stdout", 0, NULL, 'c'},
	{"version", 0, NULL, 'v'},
	{NULL, 0, NULL, 0}
    };

    while ((ret = getopt_long(argc, argv, "chpv", long_options, 0)) != -1) {
	switch (ret) {
	case 'c':
	    use_stdout = 1;
	    break;
	case 'h':
	    printf("\n");
	    printf("Usage: amigadepacker [-c] [-h] [-p] [-v] FILE ...\n");
	    printf("\n");
	    printf(" -c     Unpack to stdout.\n");
	    printf(" -h     Print this.\n");
	    printf(" -p     Do not depack anything, just pretend to. Useful for searching packed\n");
	    printf("        files. Names of packed files will be printed to stderr. Pretend mode\n");
	    printf("        always returns success if arguments are valid.\n");
	    printf(" -v     Print version information.\n");
	    printf("\n");
	    printf("Example 1: Depack file:\n");
	    printf("\tamigadepacker foo\n");
	    printf("\n");
	    printf("Example 2: Depack file from stdin to stdout:\n");
	    printf("\tamigadepacker -c < foo > outfile\n");
	    printf("\n");
	    return 0;
	case 'p':
	    pretend = 1;
	    break;
	case 'v':
	    printf("\n");
	    printf("amigadepacker v%s by Heikki Orsila <heikki.orsila@iki.fi>\n", VERSION);
	    printf("Web page: http://www.iki.fi/shd/foss/amigadepacker/\n");
	    printf("\n");
	case '?':
	case ':':
	    exit(-1);
	default:
	    fprintf(stderr, "Impossible option.\n");
	    exit(-1);
	}
    }

    if (use_stdout && optind < (argc - 1)) {
	fprintf(stderr, "At most one file parameter must be given in stdout mode.\n");
	return -1;
    }

    if ((use_stdout) && optind == argc) {
	res = decrunch("", stdout, pretend);
    } else {
	res = 0;
	for (i = optind; i < argc; i++) {
	    if (decrunch(argv[i], use_stdout ? stdout : NULL, pretend) < 0)
		res = -1;
	}
    }

    if (pretend)
      return 0;

    return res;
}
