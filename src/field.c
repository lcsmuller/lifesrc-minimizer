#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "field.h"

struct golsat_field *
golsat_field_create(CMergeSat *s,
                    int width,
                    int height,
                    struct golsat_field_init *init)
{
    if (!init) return NULL;

    if (init->var == 0) {
        init->var = 1;
    }

    struct golsat_field *field = (struct golsat_field *)malloc(sizeof *field);
    if (!field) return NULL;

    field->width = width;
    field->height = height;
    field->literals = (int *)malloc(width * height * sizeof *field->literals);
    if (!field->literals) {
        free(field);
        return NULL;
    }

    // Create a false literal
    field->is_false = init->var++;
    cmergesat_add(s, -field->is_false);
    cmergesat_add(s, 0);
    // Create literals for each cell
    for (int i = 0; i < width * height; ++i) {
        field->literals[i] = init->var++;
    }

    return field;
}

void
golsat_field_cleanup(struct golsat_field *field)
{
    free(field->literals);
    free(field);
}

int
golsat_field_get_lit(const struct golsat_field *field, int x, int y)
{
    return (x < 0 || x >= field->width || y < 0 || y >= field->height)
               ? field->is_false
               : field->literals[x + y * field->width];
}

int
golsat_field_count_true_lit(CMergeSat *s, const struct golsat_field *field)
{
    int count = 0;
    for (int i = 0; i < field->width * field->height; ++i) {
        if (cmergesat_val(s, field->literals[i]) > 0) {
            count++;
        }
    }
    return count;
}

static int
_golsat_field_validate_rules(CMergeSat *s,
                             const struct golsat_field *prev,
                             const struct golsat_field *current)
{
    for (int i = 0; i < prev->width; ++i) {
        for (int j = 0; j < prev->height; ++j) {
            int live_neighbors = 0;
            for (int x = i - 1; x <= i + 1; ++x) {
                for (int y = j - 1; y <= j + 1; ++y) {
                    if (x == i && y == j) continue;

                    const int lit = golsat_field_get_lit(prev, x, y);
                    if (lit == prev->is_false) continue;

                    live_neighbors += cmergesat_val(s, lit) > 0;
                }
            }

            const int current_state =
                          cmergesat_val(s, golsat_field_get_lit(current, i, j))
                          > 0,
                      prev_state =
                          cmergesat_val(s, golsat_field_get_lit(prev, i, j))
                          > 0;
            // Rule 1: Any live cell with fewer than 2 or more than 3 live
            //  neighbors dies
            // Rule 2: Any live cell with 2 or 3 live neighbors lives
            // Rule 3: Any dead cell with exactly 3 live neighbors
            //  becomes alive
            int should_live =
                prev_state ? (live_neighbors == 2 || live_neighbors == 3)
                           : (live_neighbors == 3);
            if (current_state != should_live) {
                fprintf(stderr,
                        "Rule violation at (%d,%d): prev=%d, neighbors=%d, "
                        "current=%d\n",
                        i, j, prev_state, live_neighbors, current_state);
                return 0;
            }
        }
    }
    return 1;
}

void
golsat_field_print(CMergeSat *s,
                   const struct golsat_field *prev,
                   const struct golsat_field *current,
                   FILE *stream)
{
    for (int y = 0; y < current->height; ++y) {
        for (int x = 0; x < current->width; ++x) {
            const int lit = golsat_field_get_lit(current, x, y),
                      ret_lit = cmergesat_val(s, lit);
            // "." if cell is dead, "X" if cell is alive
            fprintf(stream, ret_lit == -lit ? "." : "X");
        }
        fprintf(stream, "\n");
    }
    if (prev) {
        fprintf(stream, "Validating Game of Life rules... %s\n",
                _golsat_field_validate_rules(s, prev, current) ? "VALID"
                                                               : "INVALID");
    }
    fprintf(stream, "%d# alive cells\n",
            golsat_field_count_true_lit(s, current));
}
