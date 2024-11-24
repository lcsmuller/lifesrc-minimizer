#ifndef GOLSAT_FORMULA_H
#define GOLSAT_FORMULA_H

struct golsat_field;
struct golsat_pattern;

void golsat_formula_transition(CMergeSat *s,
                               const struct golsat_field *current,
                               const struct golsat_field *next);

void golsat_formula_constraint(CMergeSat *s,
                               const struct golsat_field *field,
                               const struct golsat_pattern *pat);

#endif /* !GOLSAT_FORMULA_H */
