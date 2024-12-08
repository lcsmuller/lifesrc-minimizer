#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <sys/wait.h>
#include <errno.h>

#include "commandline.h"
#include "pattern.h"
#include "popen2.h"

#define LIFESRC_START_SEARCH " \n"
#define LIFESRC_QUIT         "q\ny\n"
#define LIFESRC_EXEC         LIFESRC_START_SEARCH LIFESRC_QUIT
#define LIFESRC_EXEC_LEN     (sizeof(LIFESRC_EXEC) - 1)

#define TMPFILE_NAME "tmp.txt"

#define MAX_TOTAL_TIME_SECS 8 * 60

struct _golsat_next {
    char *last_state;
    int live_cells;
};

struct _golsat_timeout {
    time_t start_time;
    int remaining_total;
    int unused_time;
};

static int
_golsat_next_timeout(const struct golsat_pattern *pat,
                     struct _golsat_timeout *timer)
{
    const int total_cells = pat->width * pat->height,
              max_iterations = (int)(log2(total_cells) + 1);
    return (timer->remaining_total / max_iterations) + timer->unused_time;
}

static struct _golsat_next
_golsat_next_search(const char command[1024])
{
    struct popen2 exec = { 0 };
    ssize_t bytes_read;
    int exec_status;

    char output[2048] = { 0 };
    struct _golsat_next result = { NULL, 0 };

    if (popen2(command, &exec) != 0) {
        perror("popen2");
        return result;
    }

    write(exec.to_child, LIFESRC_EXEC, LIFESRC_EXEC_LEN);
    close(exec.to_child);

    while ((bytes_read = read(exec.from_child, output, sizeof(output) - 1))
           > 0) {
        output[bytes_read] = '\0';

        if ((sscanf(output, "Found object (gen %*d, cells %d",
                    &result.live_cells))) {
            const size_t start_idx = strcspn(output, "\n") + 1,
                         length = strspn(output + start_idx, "OX1.0 \n");
            result.last_state = strndup(output + start_idx, length);
            if (!result.last_state) perror("strndup");
            break;
        }
        else if (!strncmp(output, "No such object",
                          sizeof("No such object") - 1)) {
            break;
        }
        else if (!strncmp(output, "Timeout", sizeof("Timeout") - 1)) {
            result.live_cells = -1;
            break;
        }
    }
    waitpid(exec.child_pid, &exec_status, 0);
    assert(WIFEXITED(exec_status) != 0 && "lifesrc terminated abnormally");

    return result;
}

static int
_golsat_convert_cnv_to_lifesrc_format(const struct golsat_pattern *pat)
{
    FILE *f_tmp;
    int y, x;

    if (!(f_tmp = fopen(TMPFILE_NAME, "wb"))) {
        perror("-- Error: tmpfile failed");
        return 0;
    }

    for (y = 0; y < pat->height; y++) {
        for (x = 0; x < pat->width; x++) {
            switch (golsat_pattern_get_cell(pat, x, y)) {
            case GOLSAT_CELLSTATE_ALIVE:
                fputc('O', f_tmp);
                break;
            case GOLSAT_CELLSTATE_DEAD:
                fputc('.', f_tmp);
                break;
            case GOLSAT_CELLSTATE_UNKNOWN:
                fputc('?', f_tmp);
                break;
            }
        }
        fputc('\n', f_tmp);
    }
    fclose(f_tmp);
    return 1;
}

int
main(int argc, char **argv)
{
    int exit_status = EXIT_FAILURE;

    struct golsat_options options = { 0 };
    struct golsat_pattern *pat = NULL;
    struct _golsat_next next;

    char command[1024];
    FILE *f_pattern;

    int low = 0, high, mid, best_value;
    char *current_best = NULL;

    struct _golsat_timeout timer = { 0 };
    time_t iter_start, actual_time;

    timer.start_time = time(NULL);
    timer.remaining_total = MAX_TOTAL_TIME_SECS;
    timer.unused_time = 0;

    if (!golsat_commandline_parse(argc, argv, &options)) {
        return EXIT_FAILURE;
    }

    printf("-- Reading pattern from file: %s\n", options.pattern);
    if (!(f_pattern = fopen(options.pattern, "r"))) {
        fprintf(stderr, "-- Error: Cannot open %s\n", options.pattern);
        return EXIT_FAILURE;
    }

    if (!(pat = golsat_pattern_create(f_pattern, options.border_disable))) {
        fprintf(stderr, "-- Error: Pattern creation failed.\n");
        goto _cleanup_file;
    }
    high = pat->width * pat->height;

    if (!_golsat_convert_cnv_to_lifesrc_format(pat)) {
        fprintf(stderr, "-- Error: Conversion to lifesrc format failed.\n");
        goto _cleanup_pat;
    }

    while (low <= high) {
        const int timeout = _golsat_next_timeout(pat, &timer);

        mid = (low + high) / 2;

        sprintf(command,
                "timeout %d ./lifesrc -r%d -c%d -g2 -a -p -mt%d -i %s"
                " || echo 'Timeout'",
                timeout, pat->height, pat->width, mid, TMPFILE_NAME);

        printf("-- Searching for mt value: %d\t| Timeout: %d seconds\n", mid,
               timeout);

        iter_start = time(NULL);
        next = _golsat_next_search(command);
        actual_time = time(NULL) - iter_start;
        timer.unused_time = timeout - actual_time;
        timer.remaining_total =
            MAX_TOTAL_TIME_SECS - (time(NULL) - timer.start_time);

        if (timer.remaining_total <= 0) {
            fprintf(stderr, "-- Error: Total time limit reached\n");
            break;
        }

        if (next.last_state != NULL) {
            printf("\t-- Found solution for mt value: %d (took %ld secs)\n",
                   next.live_cells, actual_time);
            if (current_best) free(current_best);
            current_best = next.last_state;
            best_value = next.live_cells;
            high = next.live_cells - 1;
        }
        else {
            printf("\t-- %s for mt value: %d\n",
                   next.live_cells == -1 ? "Timeout" : "No solution", mid);
            low = mid + 1;
        }

        if (options.minimize_disable) break;
    }

    if (!current_best) {
        fprintf(stderr, "-- Error: No SAT solution found\n");
        goto _cleanup_pat;
    }

    /* the minimum value that produces a SAT solution */
    printf("-- Minimum mt value for SAT solution: %d\n%d %d\n%s", best_value,
           pat->width, pat->height, current_best);
    exit_status = EXIT_SUCCESS;

_cleanup_pat:
    if (current_best) free(current_best);
    golsat_pattern_cleanup(pat);
_cleanup_file:
    fclose(f_pattern);

    return exit_status;
}
