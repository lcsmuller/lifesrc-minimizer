#include <stdio.h>
#include <stdlib.h>

#include "field.h"

struct golsat_field *
golsat_field_create(CMergeSat *s, int width, int height)
{
    static int var = 1; // makes this function not thread-safe

    struct golsat_field *field = (struct golsat_field *)malloc(sizeof *field);
    if (!field) return NULL;

    field->m_width = width;
    field->m_height = height;
    field->m_literals =
        (int *)malloc(width * height * sizeof *field->m_literals);
    if (!field->m_literals) {
        free(field);
        return NULL;
    }

    // Create a false literal
    field->m_false = var++;
    cmergesat_add(s, -field->m_false);
    cmergesat_add(s, 0);
    // Create literals for each cell
    for (int i = 0; i < width * height; ++i) {
        field->m_literals[i] = var++;
    }

    return field;
}

void
golsat_field_cleanup(struct golsat_field *field)
{
    free(field->m_literals);
    free(field);
}

int
golsat_field_get_lit(const struct golsat_field *field, int x, int y)
{
    return (x < 0 || x >= field->m_width || y < 0 || y >= field->m_height)
               ? field->m_false
               : field->m_literals[x + y * field->m_width];
}

void
golsat_field_print(CMergeSat *s,
                   const struct golsat_field *field,
                   FILE *stream)
{
    for (int y = 0; y < field->m_height; ++y) {
        for (int x = 0; x < field->m_width; ++x) {
            const int lit = golsat_field_get_lit(field, x, y);
            int ret_lit = cmergesat_val(s, lit);
            // "." if cell is dead, "X" if cell is alive
            fprintf(stream, ret_lit != lit ? "." : "X");
        }
        fprintf(stream, "\n");
    }
}
