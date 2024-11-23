#ifndef GOLSAT_COMMANDLINE_H
#define GOLSAT_COMMANDLINE_H

typedef struct {
    int evolutions;
    int backwards;
    int grow;
    char* pattern;
} Options;

int parseCommandLine(int argc, char** argv, Options* options);

#endif /* !#GOLSAT_COMMANDLINE_H */
