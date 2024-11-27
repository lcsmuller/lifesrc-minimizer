#include <assert.h>

#include "field.h"
#include "formula.h"
#include "pattern.h"

#define MAX_NEIGHBOURS_SIZE 8

static void
_golsat_formula_add_implied(CMergeSat *s,
                            const int *clause,
                            const size_t size,
                            const int implied_lit)
{
    for (size_t i = 0; i < size; ++i) {
        cmergesat_add(s, -clause[i]);
    }
    cmergesat_add(s, implied_lit);
    cmergesat_add(s, 0);
}

static void
_golsat_formula_rule(CMergeSat *s,
                     const int current,
                     const int neighbours[MAX_NEIGHBOURS_SIZE],
                     const size_t size,
                     const int next)
{
    int clause[MAX_NEIGHBOURS_SIZE + 1];

    assert(size == MAX_NEIGHBOURS_SIZE && "unsupported number of neighbours");

    // Underpopulation (<=1 alive neighbor -> cell dies)
    for (size_t possiblyalive = 0; possiblyalive < size; ++possiblyalive) {
        size_t sz = 0;
        for (size_t dead = 0; dead < size; ++dead) {
            if (dead == possiblyalive) continue;
            clause[sz++] = -neighbours[dead];
        }
        _golsat_formula_add_implied(s, clause, sz, -next);
    }

    // Status quo (=2 alive neighbors -> cell stays alive/dead)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            size_t sz = 0;
            for (size_t i = 0; i < size; ++i) {
                clause[sz++] = (i == alive1 || i == alive2) ? neighbours[i]
                                                            : -neighbours[i];
            }
            clause[sz++] = current; // Add current cell state
            _golsat_formula_add_implied(s, clause, sz, next);
            clause[sz - 1] = -current; // Replace with negated cell
            _golsat_formula_add_implied(s, clause, sz, -next);
        }
    }

    // Birth (=3 alive neighbors -> cell becomes alive)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            for (size_t alive3 = alive2 + 1; alive3 < size; ++alive3) {
                size_t sz = 0;
                for (size_t i = 0; i < size; ++i) {
                    clause[sz++] = (i == alive1 || i == alive2 || i == alive3)
                                       ? neighbours[i]
                                       : -neighbours[i];
                }
                _golsat_formula_add_implied(s, clause, sz, next);
            }
        }
    }

    // Overpopulation (>=4 alive neighbors -> cell dies)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            for (size_t alive3 = alive2 + 1; alive3 < size; ++alive3) {
                for (size_t alive4 = alive3 + 1; alive4 < size; ++alive4) {
                    clause[0] = neighbours[alive1];
                    clause[1] = neighbours[alive2];
                    clause[2] = neighbours[alive3];
                    clause[3] = neighbours[alive4];
                    _golsat_formula_add_implied(s, clause, 4, -next);
                }
            }
        }
    }
}

void
golsat_formula_transition(CMergeSat *s,
                          const struct golsat_field *current,
                          const struct golsat_field *next)
{
    assert(current->m_width == next->m_width
           && "incompatible or unsupported field sizes");
    assert(current->m_height == next->m_height
           && "incompatible or unsupported field sizes");

    int neighbours[MAX_NEIGHBOURS_SIZE];
    for (int x = -1; x <= current->m_width; ++x) {
        for (int y = -1; y <= current->m_height; ++y) {
            size_t sz = 0;
            for (int dx = -1; dx <= +1; ++dx) {
                for (int dy = -1; dy <= +1; ++dy) {
                    if (dx == 0 && dy == 0) continue;

                    neighbours[sz++] =
                        golsat_field_get_lit(current, x + dx, y + dy);
                }
            }
            _golsat_formula_rule(s, golsat_field_get_lit(current, x, y),
                                 neighbours, sz,
                                 golsat_field_get_lit(next, x, y));
        }
    }
}

void
golsat_formula_constraint(CMergeSat *s,
                          const struct golsat_field *next,
                          const struct golsat_pattern *current_pattern)
{
    assert(next->m_width == current_pattern->width && "incompatible sizes");
    assert(next->m_height == current_pattern->height && "incompatible sizes");

    for (int x = 0; x < next->m_width; ++x) {
        for (int y = 0; y < next->m_height; ++y) {
            switch (golsat_pattern_get_cell(current_pattern, x, y)) {
            case GOLSAT_CELLSTATE_ALIVE:
                cmergesat_add(s, golsat_field_get_lit(next, x, y));
                break;
            case GOLSAT_CELLSTATE_DEAD:
                cmergesat_add(s, -golsat_field_get_lit(next, x, y));
                break;
            case GOLSAT_CELLSTATE_UNKNOWN:
                continue;
            default:
                assert(0 && "invalid cell state");
            }
            cmergesat_add(s, 0);
        }
    }
}

static int
_golsat_formula_minimize_true_literals(CMergeSat *s,
                                       struct golsat_field *field,
                                       int *min_true_literals,
                                       const int row,
                                       const int col)
{
    // Base case: reached the end of matrix (SAT)
    if (row == field->m_width) return 1;

    // Base case: UNSAT
    const int retval = cmergesat_solve(s);
    if (retval != 10) return 0;

    // Base case: current solution true_literals >= min_literals
    //  (SAT but ignore)
    const int current_true_literals = golsat_field_count_true_lit(s, field);
    if (current_true_literals > *min_true_literals) return 0;

    *min_true_literals = current_true_literals;

    const int next_row = (col == field->m_width - 1) ? row + 1 : row,
              next_col = (col == field->m_width - 1) ? 0 : col + 1;
    const int lit = golsat_field_get_lit(field, row, col);
    if (lit == field->m_false) return 0;

    cmergesat_assume(s, -lit);
    cmergesat_freeze(s, lit);
#if 0
    fprintf(stderr, "1 - (%d, %d, %d) %d\n", row, col, current_true_literals,
            -lit);
#endif
    if (_golsat_formula_minimize_true_literals(s, field, min_true_literals,
                                               next_row, next_col))
    {
        return 1;
    }
    cmergesat_melt(s, lit);

    cmergesat_assume(s, lit);
    cmergesat_freeze(s, lit);
#if 0
    fprintf(stderr, "2 - (%d, %d, %d) %d\n", row, col, current_true_literals,
            lit);
#endif
    if (_golsat_formula_minimize_true_literals(s, field, min_true_literals,
                                               next_row, next_col))
    {
        return 1;
    }
    cmergesat_melt(s, lit);

    return 1;
}

int
golsat_formula_minimize_true_literals(CMergeSat *s, struct golsat_field *field)
{
    int min_true_literals = golsat_field_count_true_lit(s, field);
    return _golsat_formula_minimize_true_literals(s, field, &min_true_literals,
                                                  0, 0);
}
