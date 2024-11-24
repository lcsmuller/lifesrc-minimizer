#ifndef GOLSAT_FIELD_H
#define GOLSAT_FIELD_H

#include <stdio.h>

#include <simp/cmergesat.h>

struct golsat_field {
    int m_width;
    int m_height;
    int *m_literals;
    int m_false;
};

struct golsat_field *golsat_field_create(CMergeSat *s, int width, int height);
void golsat_field_cleanup(struct golsat_field *field);
int golsat_field_get_lit(const struct golsat_field *field, int x, int y);
void golsat_field_print(CMergeSat *s,
                        const struct golsat_field *field,
                        FILE *stream);

#endif /* !GOLSAT_FIELD_H */
