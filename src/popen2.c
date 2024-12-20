#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "popen2.h"

int
popen2(const char *cmdline, struct popen2 *childinfo)
{
    int pipe_stdin[2], pipe_stdout[2];
    pid_t p;

    if (pipe(pipe_stdin)) return -1;
    if (pipe(pipe_stdout)) return -1;
#ifdef GOLSAT_DEBUG
    fprintf(stderr, "pipe_stdin[0] = %d, pipe_stdin[1] = %d\n", pipe_stdin[0],
            pipe_stdin[1]);
    fprintf(stderr, "pipe_stdout[0] = %d, pipe_stdout[1] = %d\n",
            pipe_stdout[0], pipe_stdout[1]);
#endif
    p = fork();
    if (p < 0) return p;
    if (p == 0) {
        close(pipe_stdin[1]);
        dup2(pipe_stdin[0], 0);
        close(pipe_stdout[0]);
        dup2(pipe_stdout[1], 1);
        execl("/bin/sh", "sh", "-c", cmdline, 0);
        perror("execl");
        exit(99);
    }
    childinfo->child_pid = p;
    childinfo->to_child = pipe_stdin[1];
    childinfo->from_child = pipe_stdout[0];

    return 0;
}
