#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "decrunch.h"

int main(int argc, char **argv)
{
    int ret, res, i;
    FILE *in, *out;
    int use_stdout = 0;

    struct option long_options[] = {
	{"help", 0, NULL, 'd'},
	{"stdout", 0, NULL, 'c'},
	{NULL, 0, NULL, 0}
    };

    while ((ret = getopt_long(argc, argv, "ch", long_options, 0)) != -1) {
	switch (ret) {
	case 'c':
	    use_stdout = 1;
	    break;
	case 'h':
	    printf("Usage: amigadepacker [-h] FILE ...\n");
	    return 0;
	case '?':
	case ':':
	    exit(-1);
	default:
	    fprintf(stderr, "Impossible option.\n");
	    exit(-1);
	}
    }

    res = 0;

    if (use_stdout && optind < (argc - 1)) {
	fprintf(stderr, "At most one file parameter must be given in stdout mode.\n");
	return -1;
    }

    for (i = optind; i < argc; i++) {
	in = fopen(argv[i], "r");
	if (in == NULL) {
	    fprintf(stderr, "Can not open %s.\n", argv[i]);
	    continue;
	}
	printf("Decrunching %s\n", argv[i]);
	if (use_stdout) {
	    out = stdout;
	} else {
	    out = fopen("temp", "w");
	}
	ret = decrunch(out, in);
	if (ret < 0)
	    res = -1;
    }

    return res;
}
