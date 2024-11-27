#ifndef GOLSAT_FIELD_H
#define GOLSAT_FIELD_H

#include <stdio.h>

#include <simp/cmergesat.h>

struct golsat_field {
    int m_width;
    int m_height;
    int m_false;
    int *m_literals;
};

struct golsat_field_init {
    int var;
};

struct golsat_field *golsat_field_create(CMergeSat *s,
                                         int width,
                                         int height,
                                         struct golsat_field_init *init);
void golsat_field_cleanup(struct golsat_field *field);
int golsat_field_get_lit(const struct golsat_field *field, int x, int y);
int golsat_field_count_true_lit(CMergeSat *s,
                                const struct golsat_field *field);
void golsat_field_print(CMergeSat *s,
                        const struct golsat_field *prev,
                        const struct golsat_field *current,
                        FILE *stream);

#endif /* !GOLSAT_FIELD_H */
