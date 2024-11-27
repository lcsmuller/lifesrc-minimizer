#ifndef GOLSAT_COMMANDLINE_H
#define GOLSAT_COMMANDLINE_H

struct golsat_options {
    int evolutions;
    char *pattern;
    int disable_minimize;
};

int golsat_commandline_parse(int argc,
                             char **argv,
                             struct golsat_options *options);

#endif /* !#GOLSAT_COMMANDLINE_H */
