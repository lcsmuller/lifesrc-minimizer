#include <stdio.h>
#include <stdlib.h>

#include "field.h"

Field* Field_create(CMergeSat* s, int width, int height) {
    static int var = 1;

    Field* field = (Field*)malloc(sizeof *field);
    field->m_width = width;
    field->m_height = height;
    field->m_literals =
        (int*)malloc(width * height * sizeof *field->m_literals);

    field->m_false = var++;
    cmergesat_add(s, -field->m_false);
    cmergesat_add(s, 0);

    for (int i = 0; i < width * height; ++i) {
        field->m_literals[i] = var++;
    }

    return field;
}

void Field_destroy(Field* field) {
    free(field->m_literals);
    free(field);
}

int Field_get_lit(const Field* field, int x, int y) {
    if (x < 0 || x >= field->m_width || y < 0 || y >= field->m_height) {
        return field->m_false;
    }
    return field->m_literals[x + y * field->m_width];
}

void Field_print(CMergeSat* s, const Field* field, FILE* stream) {
    for (int y = 0; y < field->m_height; ++y) {
        for (int x = 0; x < field->m_width; ++x) {
            const int lit = Field_get_lit(field, x, y);
            int ret_lit = cmergesat_val(s, lit);
            if (ret_lit != lit) { /* l_False */
                fprintf(stream, ".");
            } else { /* l_True */
                fprintf(stream, "X");
            }
        }
        fprintf(stream, "\n");
    }
}
