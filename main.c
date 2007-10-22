#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "version.h"
#include "decrunch.h"

static char usage[] =
"\n"
"Usage: amigadepacker [-c] [-h] [-o out] [-p] [-v] FILE ...\n"
"\n"
" -c, --stdout   Unpack to stdout\n"
" -h, --help     Print this help\n"
" -o out, --output-file=out\n"
"                Unpack to file named \"out\"\n"
" -p, --pretend  Do not depack anything, just pretend. Useful for searching\n"
"                packed files. Names of packed files will be printed to stderr.\n"
"                Pretending mode always returns success if arguments are valid.\n"
" -v, --version  Print version information\n"
"\n"
"Example 1: Depack file, replace \"foo\" with an unpacked version:\n"
"\tamigadepacker foo\n"
"\n"
"Example 2: Depack file from stdin to stdout:\n"
"\tamigadepacker < foo > outfile\n"
"\n";


static int handle_file(char *output_file, char *input_file, int inplace,
		       int pretend)
{
    FILE *out;
    int ret = 0;

    if (output_file) {
	out = fopen(output_file, "wb");
	if (out == NULL) {
	    fprintf(stderr, "Can not open %s for writing: %s\n", output_file, strerror(errno));
	    return 1;
	}

    } else if (inplace) {
	out = NULL;

    } else {
	out = stdout;
    }

    if (decrunch(out, input_file, pretend) < 0) {
	if (output_file)
	    unlink(output_file);
	
	ret = 1;
    }

    if (out != NULL)
	fclose(out);

    return ret;
}


int main(int argc, char **argv)
{
    int ret, i;
    int use_stdout = 0;
    int nfiles = 0;
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
	    printf("%s", usage);
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

    nfiles = (optind == argc) ? 0 : argc - optind;

    if (use_stdout && output_file != NULL) {
	fprintf(stderr, "You may not use both -c and -o\n");
	return 1;
    }

    if ((use_stdout || output_file != NULL) && nfiles > 1) {
	fprintf(stderr, "At most one file parameter can be given in stdout mode,\n"
		"or when -o output_file is given.\n");
	return 1;
    }

    if (nfiles == 0) {
	ret = handle_file(output_file, NULL, 0, pretend);

    } else {
	ret = 0;
	for (i = optind; i < argc; i++)
	    ret |= handle_file(output_file, argv[i], !use_stdout, pretend);
    }

    if (pretend)
	return 0;

    return ret;
}
