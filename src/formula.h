#ifndef GOLSAT_FORMULA_H
#define GOLSAT_FORMULA_H

struct golsat_field;
struct golsat_pattern;

void golsat_formula_transition(CMergeSat *s,
                               const struct golsat_field *current,
                               const struct golsat_field *next);

void golsat_formula_constraint(CMergeSat *s,
                               const struct golsat_field *next,
                               const struct golsat_pattern *current_pattern);

int golsat_formula_minimize_true_literals(CMergeSat *s,
                                          struct golsat_field *field);

#endif /* !GOLSAT_FORMULA_H */
