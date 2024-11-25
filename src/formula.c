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
    assert(size == MAX_NEIGHBOURS_SIZE);

    // Underpopulation (<=1 alive neighbor -> cell dies)
    for (size_t possiblyalive = 0; possiblyalive < size; ++possiblyalive) {
        int *cond = (int *)malloc((size - 1) * sizeof(int));
        size_t sz = 0;
        for (size_t dead = 0; dead < size; ++dead) {
            if (dead == possiblyalive) continue;
            cond[sz++] = -neighbours[dead];
        }
        _golsat_formula_add_implied(s, cond, sz, -next);
        free(cond);
    }

    // Status quo (=2 alive neighbors -> cell stays alive/dead)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            int *cond = (int *)malloc((size + 1) * sizeof(int));
            size_t sz = 0;
            for (size_t i = 0; i < size; ++i) {
                cond[sz++] = (i == alive1 || i == alive2) ? neighbours[i]
                                                          : -neighbours[i];
            }
            cond[sz++] = current; // Add current cell state
            _golsat_formula_add_implied(s, cond, sz, next);
            cond[sz - 1] = -current; // Replace with negated cell
            _golsat_formula_add_implied(s, cond, sz, -next);
            free(cond);
        }
    }

    // Birth (=3 alive neighbors -> cell becomes alive)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            for (size_t alive3 = alive2 + 1; alive3 < size; ++alive3) {
                int *cond = (int *)malloc(size * sizeof(int));
                size_t sz = 0;
                for (size_t i = 0; i < size; ++i) {
                    cond[sz++] = (i == alive1 || i == alive2 || i == alive3)
                                     ? neighbours[i]
                                     : -neighbours[i];
                }
                _golsat_formula_add_implied(s, cond, sz, next);
                free(cond);
            }
        }
    }

    // Overpopulation (>=4 alive neighbors -> cell dies)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            for (size_t alive3 = alive2 + 1; alive3 < size; ++alive3) {
                for (size_t alive4 = alive3 + 1; alive4 < size; ++alive4) {
                    int cond[] = { neighbours[alive1], neighbours[alive2],
                                   neighbours[alive3], neighbours[alive4] };
                    _golsat_formula_add_implied(
                        s, cond, sizeof(cond) / sizeof(cond[0]), -next);
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
    int offset_x = 0;
    int offset_y = 0;
    int from_x, to_x, from_y, to_y;

    if ((current->m_width == next->m_width)
        && (current->m_height == next->m_height))
    {
        // same field size
        from_x = -1;
        to_x = current->m_width;
        from_y = -1;
        to_y = current->m_height;
    }
    else if ((current->m_width + 2 == next->m_width)
             && (current->m_height + 2 == next->m_height))
    {
        // field size expands
        from_x = -2;
        to_x = current->m_width + 1;
        from_y = -2;
        to_y = current->m_height + 1;
        offset_x = 1;
        offset_y = 1;
    }
    else if ((current->m_width == next->m_width + 2)
             && (current->m_height == next->m_height + 2))
    {
        // field size shrinks
        from_x = -1;
        to_x = current->m_width;
        from_y = -1;
        to_y = current->m_height;
        offset_x = -1;
        offset_y = -1;
    }
    else {
        assert(0 && "incompatible field sizes");
    }

    int neighbours[MAX_NEIGHBOURS_SIZE];
    for (int x = from_x; x <= to_x; ++x) {
        for (int y = from_y; y <= to_y; ++y) {
            size_t sz = 0;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;

                    neighbours[sz++] =
                        golsat_field_get_lit(current, x + dx, y + dy);
                }
            }
            _golsat_formula_rule(
                s, golsat_field_get_lit(current, x, y), neighbours, sz,
                golsat_field_get_lit(next, x + offset_x, y + offset_y));
        }
    }
}

void
golsat_formula_constraint(CMergeSat *s,
                          const struct golsat_field *field,
                          const struct golsat_pattern *pat)
{
    assert(field->m_width == pat->width);
    assert(field->m_height == pat->height);

    for (int x = 0; x < field->m_width; ++x) {
        for (int y = 0; y < field->m_height; ++y) {
            switch (golsat_pattern_get_cell(pat, x, y)) {
            case GOLSAT_CELLSTATE_ALIVE:
                cmergesat_add(s, golsat_field_get_lit(field, x, y));
                break;
            case GOLSAT_CELLSTATE_DEAD:
                cmergesat_add(s, -golsat_field_get_lit(field, x, y));
                break;
            case GOLSAT_CELLSTATE_UNKNOWN:
                break;
            }
            cmergesat_add(s, 0);
        }
    }
}
