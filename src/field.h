#ifndef GOLSAT_FIELD_H
#define GOLSAT_FIELD_H

#include <stdio.h>

#include <simp/cmergesat.h>

typedef struct {
    int m_width;
    int m_height;
    int* m_literals;
    int m_false;
} Field;

Field* Field_create(CMergeSat* s, int width, int height);
void Field_destroy(Field* field);
int Field_get_lit(const Field* field, int x, int y);
void Field_print(CMergeSat* s, const Field* field, FILE* stream);

#endif /* !GOLSAT_FIELD_H */
