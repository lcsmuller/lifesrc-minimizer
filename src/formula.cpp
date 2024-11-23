#include <assert.h>
#include <set>
#include <vector>

#include "field.h"
#include "formula.h"
#include "pattern.h"

static void addImplied(CMergeSat* s, const int* clause, const size_t size,
                       const int implied_lit) {
    for (size_t i = 0; i < size; ++i) {
        cmergesat_add(s, -clause[i]);
    }
    cmergesat_add(s, implied_lit);
    cmergesat_add(s, 0);
}

static void rule(CMergeSat* s, const int cell, const int n[], const size_t size,
                 const int next) {
    assert(size == 8);

    // Underpopulation (<=1 alive neighbor -> cell dies)
    for (size_t possiblyalive = 0; possiblyalive < size; ++possiblyalive) {
        int* cond = (int*)malloc((size - 1) * sizeof(int));
        size_t sz = 0;
        for (size_t dead = 0; dead < size; ++dead) {
            if (dead == possiblyalive)
                continue;
            cond[sz++] = -n[dead];
        }
        addImplied(s, cond, sz, -next);
        free(cond);
    }

    // Status quo (=2 alive neighbors -> cell stays alive/dead)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            int* cond = (int*)malloc((size + 1) * sizeof(int));
            size_t sz = 0;
            for (size_t i = 0; i < size; ++i) {
                cond[sz++] = (i == alive1 || i == alive2) ? n[i] : -n[i];
            }
            cond[sz++] = cell; // Add current cell state
            addImplied(s, cond, sz, next);
            cond[sz - 1] = -cell; // Replace with negated cell
            addImplied(s, cond, sz, -next);
            free(cond);
        }
    }

    // Birth (=3 alive neighbors -> cell becomes alive)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            for (size_t alive3 = alive2 + 1; alive3 < size; ++alive3) {
                int* cond = (int*)malloc(size * sizeof(int));
                size_t sz = 0;
                for (size_t i = 0; i < size; ++i) {
                    cond[sz++] = (i == alive1 || i == alive2 || i == alive3)
                                     ? n[i]
                                     : -n[i];
                }
                addImplied(s, cond, sz, next);
                free(cond);
            }
        }
    }

    // Overpopulation (>=4 alive neighbors -> cell dies)
    for (size_t alive1 = 0; alive1 < size; ++alive1) {
        for (size_t alive2 = alive1 + 1; alive2 < size; ++alive2) {
            for (size_t alive3 = alive2 + 1; alive3 < size; ++alive3) {
                for (size_t alive4 = alive3 + 1; alive4 < size; ++alive4) {
                    int cond[] = {n[alive1], n[alive2], n[alive3], n[alive4]};
                    addImplied(s, cond, sizeof(cond) / sizeof(cond[0]), -next);
                }
            }
        }
    }
}

void transition(CMergeSat* s, const Field* current, const Field* next) {
    int offset_x = 0;
    int offset_y = 0;
    int from_x, to_x, from_y, to_y;

    if ((current->m_width == next->m_width) &&
        (current->m_height == next->m_height)) {
        // same field size
        from_x = -1;
        to_x = current->m_width;
        from_y = -1;
        to_y = current->m_height;
    } else if ((current->m_width + 2 == next->m_width) &&
               (current->m_height + 2 == next->m_height)) {
        // field size expands
        from_x = -2;
        to_x = current->m_width + 1;
        from_y = -2;
        to_y = current->m_height + 1;
        offset_x = 1;
        offset_y = 1;
    } else if ((current->m_width == next->m_width + 2) &&
               (current->m_height == next->m_height + 2)) {
        // field size shrinks
        from_x = -1;
        to_x = current->m_width;
        from_y = -1;
        to_y = current->m_height;
        offset_x = -1;
        offset_y = -1;
    } else {
        assert(false && "incompatible field sizes");
    }

    for (int x = from_x; x <= to_x; ++x) {
        for (int y = from_y; y <= to_y; ++y) {
            int* neighbours = (int*)malloc(8 * sizeof(int));
            size_t sz = 0;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0)
                        continue;
                    neighbours[sz++] = Field_get_lit(current, x + dx, y + dy);
                }
            }
            rule(s, Field_get_lit(current, x, y), neighbours, sz,
                 Field_get_lit(next, x + offset_x, y + offset_y));
            free(neighbours);
        }
    }
}

void patternConstraint(CMergeSat* s, const Field* field, const Pattern& pat) {
    assert(field->m_width == pat.width());
    assert(field->m_height == pat.height());

    for (int x = 0; x < field->m_width; ++x) {
        for (int y = 0; y < field->m_height; ++y) {
            switch (pat(x, y)) {
            case Pattern::CellState::Alive:
                cmergesat_add(s, Field_get_lit(field, x, y));
                break;
            case Pattern::CellState::Dead:
                cmergesat_add(s, -Field_get_lit(field, x, y));
                break;
            case Pattern::CellState::Unknown:
                break;
            }
            cmergesat_add(s, 0);
        }
    }
}
