#include <stdio.h>
#include <stdlib.h>

#include "commandline.h"
#include "field.h"
#include "formula.h"
#include "pattern.h"

#ifdef DEBUG_MODE
void
learnCallback(void *state, int *clause)
{
    (void)state;
    printf("Learned clause: ");
    while (*clause != 0) { // Clause terminator
        printf("%d ", *clause);
        ++clause;
    }
    printf("\n");
}
#endif // DEBUG_MODE

int
main(int argc, char **argv)
{
    struct golsat_options options = { 0 };
    if (!golsat_commandline_parse(argc, argv, &options)) {
        return EXIT_FAILURE;
    }

    printf("-- Reading pattern from file: %s\n", options.pattern);
    FILE *f = fopen(options.pattern, "r");
    if (!f) {
        printf("-- Error: Cannot open %s\n", options.pattern);
        return 1;
    }

    struct golsat_pattern *pat = golsat_pattern_create(f);
    if (!pat) {
        fprintf(stderr, "-- Error: Pattern creation failed.\n");
        return EXIT_FAILURE;
    }

    CMergeSat *s = cmergesat_init();
#ifdef DEBUG_MODE
    cmergesat_set_learn(s, NULL, 2, learnCallback);
#endif // DEBUG_MODE

    printf("-- Building formula for %d evolution steps...\n",
           options.evolutions);
    struct golsat_field **fields = (struct golsat_field **)malloc(
        sizeof(struct golsat_field *) * (options.evolutions + 1));
    if (!fields) {
        fprintf(stderr, "-- Error: Memory allocation for fields failed.\n");
        cmergesat_release(s);
        return EXIT_FAILURE;
    }

    for (int g = 0; g <= options.evolutions; ++g) {
        // Create field for the current generation
        fields[g] = golsat_field_create(s, pat->width, pat->height);
        if (!fields[g]) {
            fprintf(stderr,
                    "-- Error: Field creation failed for generation %d.\n", g);
            for (int i = 0; i < g; ++i) {
                golsat_field_cleanup(fields[i]);
            }
            free(fields);
            cmergesat_release(s);
            return EXIT_FAILURE;
        }

        // Add transitions between generations
        if (g > 0) {
            golsat_formula_transition(s, fields[g - 1], fields[g]);
        }
    }

    printf("-- Setting pattern constraint on last generation...\n");
    golsat_formula_constraint(s, fields[options.evolutions], pat);

    printf("-- Solving formula...\n");
    if (!cmergesat_solve(s)) {
        fprintf(stderr,
                "-- Formula is not solvable. The selected pattern is probably "
                "too restrictive!\n");
        for (int g = 0; g <= options.evolutions; ++g) {
            golsat_field_cleanup(fields[g]);
        }
        free(fields);
        cmergesat_release(s);
        return EXIT_FAILURE;
    }

    printf("\n");
    for (int g = 0; g <= options.evolutions; ++g) {
        if (g == 0) {
            printf("-- Initial generation:\n");
        }
        else if (g == options.evolutions) {
            printf("-- Evolves to final generation (from pattern):\n");
        }
        else {
            printf("-- Evolves to:\n");
        }
        golsat_field_print(s, fields[g], stdout);
        printf("\n");
    }

    for (int g = 0; g <= options.evolutions; ++g) {
        golsat_field_cleanup(fields[g]);
    }
    free(fields);
    cmergesat_release(s);

    return EXIT_SUCCESS;
}
