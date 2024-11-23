#ifndef GOLSAT_FORMULA_H
#define GOLSAT_FORMULA_H

class Pattern;

void transition(CMergeSat* s, const Field* current, const Field* next);
void patternConstraint(CMergeSat* s, const Field* field, const Pattern& pat);

#endif /* !GOLSAT_FORMULA_H */
