#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "version.h"
#include "decrunch.h"

int main(int argc, char **argv)
{
    int ret, res, i;
    int use_stdout = 0;
    int pretend = 0;
    char *output_file = NULL;

    struct option long_options[] = {
	{"help", 0, NULL, 'h'},
	{"pretend", 0, NULL, 'p'},
	{"output-file", 1, NULL, 'o'},
	{"stdout", 0, NULL, 'c'},
	{"version", 0, NULL, 'v'},
	{NULL, 0, NULL, 0}
    };

    while ((ret = getopt_long(argc, argv, "cho:pv", long_options, NULL)) != -1) {
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

	case 'o':
	  output_file = strdup(optarg);
	  if (output_file == NULL) {
	      perror("amigadepacker: output file name");
	      exit(1);
	  }
	  break;

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
	    exit(1);

	default:
	    fprintf(stderr, "amigadepacker: Impossible option.\n");
	    exit(1);
	}
    }

    if (use_stdout && output_file != NULL) {
	fprintf(stderr, "You may not use both -c and -o\n");
	return 1;
    }

    if ((use_stdout || output_file != NULL) && optind < (argc - 1)) {
	fprintf(stderr, "At most one file parameter can be given in stdout mode,\n"
		"or when -o output_file is given.\n");
	return 1;
    }

    if (use_stdout && optind == argc) {
	res = decrunch("", stdout, pretend);
    } else {
	res = 0;
	for (i = optind; i < argc; i++) {
	    FILE *out = NULL;

	    if (use_stdout) {
		out = stdout;

	    } else if (output_file != NULL) {
		out = fopen(output_file, "wb");
		if (out == NULL) {
		    fprintf(stderr, "Can not open %s for writing: %s\n", output_file, strerror(errno));
		    exit(1);
		}
	    }

	    if (decrunch(argv[i], out, pretend) < 0) {

		if (output_file != NULL)
		    unlink(output_file);

		res = 1;
	    }
	}
    }

    if (pretend)
	return 0;

    return res;
}
