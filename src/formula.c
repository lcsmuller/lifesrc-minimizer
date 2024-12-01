#include <assert.h>
#include <string.h>
#include <time.h>

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
_golsat_formula_minimize_alive(CMergeSat *s,
                               struct golsat_field *field,
                               int *min_alive_cells,
                               const int row,
                               const int col,
                               int alive_cells_count)
{
    // Base case: unsatisfiable
    if (cmergesat_solve(s) != 10) {
        return 0;
    }
    // Base case: cell cannot be retrieved (out of bounds)
    const int lit = golsat_field_get_lit(field, row, col);
    if (lit == field->m_false) return 0;

    const int next_row = (col == field->m_width - 1) ? row + 1 : row,
              next_col = (col == field->m_width - 1) ? 0 : col + 1;

    // TODO add heuristic to trim the search space
    if (alive_cells_count + (-lit > 0) < *min_alive_cells) {
        cmergesat_assume(s, -lit);
        cmergesat_freeze(s, lit);
        if (_golsat_formula_minimize_alive(s, field, min_alive_cells, next_row,
                                           next_col,
                                           alive_cells_count + (-lit > 0)))
            return 1;
        cmergesat_melt(s, lit);
    }
    // TODO add heuristic to trim the search space
    if (alive_cells_count + (lit > 0) < *min_alive_cells) {
        cmergesat_assume(s, lit);
        cmergesat_freeze(s, lit);
        if (_golsat_formula_minimize_alive(s, field, min_alive_cells, next_row,
                                           next_col,
                                           alive_cells_count + (lit > 0)))
            return 1;
        cmergesat_melt(s, lit);
    }

    // Base case: minimized solution doesn't improve current best
    //  (SAT but ignore)
    const int current_alive_cells = golsat_field_count_true_lit(s, field);
    if (current_alive_cells >= *min_alive_cells) {
#if 0
        printf("-- Ignoring %d true literals (best: %d)\n",
               current_alive_cells, *min_alive_cells);
#endif
        return 0;
    }

    *min_alive_cells = current_alive_cells;

    return 1;
}

int
golsat_formula_minimize_alive(CMergeSat *s, struct golsat_field *field)
{
    const int size = field->m_width * field->m_height;
    int min_alive_cells = size; // start with worst case
    int *best = (int *)calloc(size, sizeof(int));

    double total_execution_time = 0.0;
    int retval = 0;
    do {
        // save current best
        for (int i = 0; i < size; ++i)
            best[i] = cmergesat_val(s, i);

        const clock_t start_time = clock();
        retval = _golsat_formula_minimize_alive(s, field, &min_alive_cells, 0,
                                                0, 0);
        const double execution_time =
            ((double)(clock() - start_time)) / CLOCKS_PER_SEC;

        printf("-- (%s) Minimized to %d alive cells\n",
               retval ? "CHANGE" : "NO CHANGE", min_alive_cells);
        printf("   Execution time: %lf seconds\n", execution_time);

        total_execution_time += execution_time;
    } while (retval != 0);
    printf("-- Total execution time: %lf seconds\n\n", total_execution_time);

    // reapply current best
    for (int i = 0; i < size; ++i)
        cmergesat_assume(s, best[i]);
    free(best);
    return cmergesat_solve(s);
}
