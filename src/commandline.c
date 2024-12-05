#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "commandline.h"

static void
_golsat_commandline_usage(char *program)
{
    printf("Usage: %s [OPTIONS]... PATTERN_FILE\n"
           "Options:\n"
           "  -h, --help             Display this help message\n"
           "  -e, --evolutions N     Set number of computed evolution steps "
           "(default is 1)\n"
           "  -d, --disableMinimize  Disable minimization of true literals "
           "(default is false)\n",
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
    options->disable_minimize = 0;

    while ((opt = getopt(argc, argv, "dhe:")) != -1) {
        switch (opt) {
        case 'e':
            options->evolutions = (int)strtol(optarg, NULL, 10);
            break;
        case 'd':
            options->disable_minimize = 1;
            break;
        case 'h':
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
