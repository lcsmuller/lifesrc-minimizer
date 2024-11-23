#include "commandline.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void usage(char* program) {
    printf("Usage: %s [OPTIONS]... PATTERN_FILE\n"
           "Options:\n"
           "  -h, --help          Display this help message\n"
           "  -f, --forward       Perform forward computation (default is "
           "backwards)\n"
           "  -g, --grow          Allow for field size growth\n"
           "  -e, --evolutions N  Set number of computed evolution steps "
           "(default is 1)\n",
           program);
}

int parseCommandLine(int argc, char** argv, Options* options) {
    int opt;
    options->evolutions = 1;
    options->backwards = 1;
    options->grow = 0;
    options->pattern = NULL;

    while ((opt = getopt(argc, argv, "hfge:")) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'f':
            options->backwards = 0;
            break;
        case 'g':
            options->grow = 1;
            break;
        case 'e':
            options->evolutions = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            return 0;
        }
    }

    if (optind < argc) {
        options->pattern = argv[optind];
    }

    if (options->pattern == NULL) {
        fprintf(stderr, "No PATTERN_FILE given\n");
        usage(argv[0]);
        return 0;
    }
    if (options->evolutions < 1) {
        fprintf(stderr, "Specified number of evolutions must be >= 1\n");
        usage(argv[0]);
        return 0;
    }

    return 1;
}
