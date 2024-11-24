#ifndef GOLSAT_COMMANDLINE_H
#define GOLSAT_COMMANDLINE_H

struct golsat_options {
    int evolutions;
    int backwards;
    int grow;
    char *pattern;
};

int golsat_commandline_parse(int argc,
                             char **argv,
                             struct golsat_options *options);

#endif /* !#GOLSAT_COMMANDLINE_H */
