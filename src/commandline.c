#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commandline.h"

static void
_golsat_commandline_usage(char *program)
{
    printf("Usage: %s [OPTIONS]... PATTERN_FILE\n"
           "Options:\n"
           "  -h, --help          Display this help message\n"
           "  -e, --evolutions N  Set number of computed evolution steps "
           "(default is 1)\n",
           program);
}

int
golsat_commandline_parse(int argc, char **argv, struct golsat_options *options)
{
    int opt;
    extern int optind;
    extern char *optarg;

    options->evolutions = 1;
    options->pattern = NULL;

    while ((opt = getopt(argc, argv, "hfge:")) != -1) {
        switch (opt) {
        case 'h':
            _golsat_commandline_usage(argv[0]);
            return 0;
        case 'e':
            options->evolutions = (int)strtol(optarg, NULL, 10);
            break;
        default:
            _golsat_commandline_usage(argv[0]);
            return 0;
        }
    }

    if (optind < argc) {
        options->pattern = argv[optind];
    }

    if (options->pattern == NULL) {
        fprintf(stderr, "No PATTERN_FILE given\n");
        _golsat_commandline_usage(argv[0]);
        return 0;
    }
    if (options->evolutions < 1) {
        fprintf(stderr, "Specified number of evolutions must be >= 1\n");
        _golsat_commandline_usage(argv[0]);
        return 0;
    }

    return 1;
}
