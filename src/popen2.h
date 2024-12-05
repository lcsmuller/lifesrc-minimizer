#ifndef POPEN2_H
#define POPEN2_H

#include <unistd.h>

struct popen2 {
    pid_t child_pid;
    int from_child, to_child;
};

int popen2(const char *cmdline, struct popen2 *childinfo);

#endif /* #!POPEN2_H */
