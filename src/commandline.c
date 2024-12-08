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
           "  -M, --minimizeDisable  Disable minimization of true literals "
           "(default is false)\n"
           "  -d, --debug            Enable debug output\n",
           program);
}

int
golsat_commandline_parse(int argc, char **argv, struct golsat_options *options)
{
    int opt;
    extern int optind;
    extern char *optarg;

    options->pattern = NULL;
    options->minimize_disable = 0;

    while ((opt = getopt(argc, argv, "dMh")) != -1) {
        switch (opt) {
        case 'M':
            options->minimize_disable = 1;
            break;
        case 'd':
            options->debug_enable = 1;
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

    return 1;
}
