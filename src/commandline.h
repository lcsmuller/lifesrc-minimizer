#ifndef GOLSAT_COMMANDLINE_H
#define GOLSAT_COMMANDLINE_H

struct golsat_options {
    char *pattern;
    int minimize_disable;
    int debug_enable;
};

int golsat_commandline_parse(int argc,
                             char **argv,
                             struct golsat_options *options);

#endif /* !#GOLSAT_COMMANDLINE_H */
