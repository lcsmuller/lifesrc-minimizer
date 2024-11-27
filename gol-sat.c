#include <stdio.h>
#include <stdlib.h>

#include "commandline.h"
#include "field.h"
#include "formula.h"
#include "pattern.h"

#ifdef DEBUG_MODE
#define DEBUG_CALL(func) func
#else
#define DEBUG_CALL(func)
#endif // DEBUG_MODE

void
learnCallback(void *state, int *clause)
{
    (void)state;
    fprintf(stderr, "Learned clause: ");
    while (*clause) {
        fprintf(stderr, "%d ", *clause++);
    }
    fprintf(stderr, "\n");
}

int
main(int argc, char **argv)
{
    int retval = EXIT_FAILURE;

    struct golsat_field **fields;
    CMergeSat *s;
    FILE *f;

    struct golsat_options options = { 0 };

    if (!golsat_commandline_parse(argc, argv, &options)) {
        return EXIT_FAILURE;
    }

    printf("-- Reading pattern from file: %s\n", options.pattern);
    f = fopen(options.pattern, "r");
    if (!f) {
        printf("-- Error: Cannot open %s\n", options.pattern);
        return EXIT_FAILURE;
    }

    struct golsat_pattern *pat = golsat_pattern_create(f);
    if (!pat) {
        fprintf(stderr, "-- Error: Pattern creation failed.\n");
        goto _cleanup_file;
    }

    printf("-- Building formula for %d evolution steps...\n",
           options.evolutions);
    fields = (struct golsat_field **)malloc(sizeof(struct golsat_field *)
                                            * (options.evolutions + 1));
    s = cmergesat_init();
    if (!fields) {
        fprintf(stderr, "-- Error: Memory allocation for fields failed.\n");
        goto _cleanup_pat;
    }

    DEBUG_CALL(
        cmergesat_set_learn(s, NULL, pat->height * pat->width, learnCallback));
    for (int g = 0; g <= options.evolutions; ++g) {
        // Create field for the current generation
        fields[g] = golsat_field_create(s, pat->width, pat->height);
        if (!fields[g]) {
            fprintf(stderr,
                    "-- Error: Field creation failed for generation %d.\n", g);
            goto _cleanup_sat;
        }

        // Add transitions between generations
        if (g > 0) {
            golsat_formula_transition(s, fields[g - 1], fields[g]);
        }
    }

    printf("-- Setting pattern constraint on last generation...\n");
    golsat_formula_constraint(s, fields[options.evolutions], pat);

    if (cmergesat_simplify(s) == 20) {
        printf("-- Error: Formula is unsatisfiable. Cannot be simplified.\n");
        goto _cleanup_sat;
    }

    printf("-- Solving formula...\n");
    switch (cmergesat_solve(s)) {
    case 10:
        printf("\n");
        for (int g = 0; g <= options.evolutions; ++g) {
            if (g == 0)
                printf("-- Initial generation:\n");
            else if (g == options.evolutions)
                printf("-- Evolves to final generation (from pattern):\n");
            else
                printf("-- Evolves to:\n");
            golsat_field_print(s, fields[g], stdout);
            printf("\n");
        }
        retval = EXIT_SUCCESS;
        break;
    case 0:
        fprintf(stderr, "-- Internal error: Solver failed.\n");
        break;
    case 20:
    default:
        fprintf(stderr,
                "-- Formula is not solvable. The selected pattern is probably "
                "too restrictive!\n");
        break;
    }
    printf("-- Formula statistics:\n");
    cmergesat_print_statistics(s);

_cleanup_sat:
    for (int g = 0; g <= options.evolutions; ++g) {
        golsat_field_cleanup(fields[g]);
    }
    free(fields);
    cmergesat_release(s);
_cleanup_pat:
    golsat_pattern_cleanup(pat);
_cleanup_file:
    fclose(f);

    return retval;
}
