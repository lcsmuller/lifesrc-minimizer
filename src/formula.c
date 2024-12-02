#include <stdlib.h>
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
    assert(current->width == next->width
           && "incompatible or unsupported field width sizes");
    assert(current->height == next->height
           && "incompatible or unsupported field height sizes");

    int neighbours[MAX_NEIGHBOURS_SIZE];
    for (int x = -1; x <= current->width; ++x) {
        for (int y = -1; y <= current->height; ++y) {
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
    assert(next->width == current_pattern->width
           && "incompatible width sizes");
    assert(next->height == current_pattern->height
           && "incompatible height sizes");

    for (int x = 0; x < next->width; ++x) {
        for (int y = 0; y < next->height; ++y) {
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
_golsat_formula_predict_alive_cells(const struct golsat_field *field,
                                    const int *runtime_states,
                                    int current_alive)
{
    int predicted_alive = current_alive;
    double weight_sum = 0.0;

    for (int i = 0; i < field->width; ++i) {
        for (int j = 0; j < field->height; ++j) {
            if (runtime_states[i + j * field->width] != 0) continue;

            int fixed_alive = 0, undefined = 0;
            double alive_probability = 0.0;

            // Check 3x3 neighborhood
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;

                    const int x = i + dx, y = j + dy;
                    if (x < 0 || x >= field->width || y < 0
                        || y >= field->height)
                        continue;

                    const int state = runtime_states[x + y * field->width];
                    if (state > 0)
                        fixed_alive++;
                    else if (state == 0)
                        undefined++;
                }
            }

            // Calculate probability based on Game of Life rules
            if (fixed_alive >= 4)
                alive_probability = 0.0;
            else if (fixed_alive == 3)
                alive_probability = 1.0;
            else if (fixed_alive == 2)
                alive_probability = 0.5;
            else if (undefined > 0)
                alive_probability = fixed_alive / 8.0;

            predicted_alive += alive_probability;
            weight_sum += 1.0;
        }
    }

    return predicted_alive + (int)(weight_sum * 0.2);
}

struct _golsat_tree {
    int lit;
    int visited;
    struct _golsat_tree *left;
    struct _golsat_tree *right;
};

static void
_golsat_tree_cleanup(struct _golsat_tree *tree)
{
    if (tree->left) _golsat_tree_cleanup(tree->left);
    if (tree->right) _golsat_tree_cleanup(tree->right);
    free(tree);
}

static struct _golsat_tree *
_golsat_tree_next(struct _golsat_tree *tree, int lit)
{
    if (!tree) {
        tree = (struct _golsat_tree *)calloc(1, sizeof *tree);
        tree->lit = lit;
        return tree;
    }

    if (tree->lit == lit) {
        return tree;
    }

    if (lit < 0) {
        if (!tree->left) {
            tree->left = (struct _golsat_tree *)calloc(1, sizeof *tree);
            tree->left->lit = lit;
        }
        return tree->left;
    }
    else if (lit > 0) {
        if (!tree->right) {
            tree->right = (struct _golsat_tree *)calloc(1, sizeof *tree);
            tree->right->lit = lit;
        }
        return tree->right;
    }
    else {
        assert(lit != 0 && "unexpected literal (only root may be lit 0)");
    }
    return tree;
}

static void
_golsat_tree_mark_visited(struct _golsat_tree *tree)
{
    tree->visited = 1;
    if (tree->left) {
        _golsat_tree_cleanup(tree->left);
        tree->left = NULL;
    }
    if (tree->right) {
        _golsat_tree_cleanup(tree->right);
        tree->right = NULL;
    }
}

static struct _golsat_tree *
_golsat_formula_next_branch(const struct golsat_field *field,
                            const int row,
                            const int col,
                            const int alive_count,
                            const int lit,
                            int *runtime_states,
                            struct _golsat_tree *tree)
{
    struct _golsat_tree *lbranch = _golsat_tree_next(tree, -lit),
                        *rbranch = _golsat_tree_next(tree, lit);

    if (lbranch->visited && rbranch->visited) {
        assert(tree->lit == 0 && "expected root");
        return NULL;
    }

    if (lbranch->visited) return rbranch;
    if (rbranch->visited) return lbranch;

    runtime_states[row * field->width + col] = lbranch->lit;
    const int left_branch_alive_cells = _golsat_formula_predict_alive_cells(
        field, runtime_states, alive_count + (lbranch->lit > 0));
    runtime_states[row * field->width + col] = 0;

    runtime_states[row * field->width + col] = rbranch->lit;
    const int right_branch_alive_cells = _golsat_formula_predict_alive_cells(
        field, runtime_states, alive_count + (rbranch->lit > 0));

    return (left_branch_alive_cells <= right_branch_alive_cells) ? lbranch
                                                                 : rbranch;
}

static int
_golsat_formula_minimize_alive(CMergeSat *s,
                               struct golsat_field *field,
                               const int row,
                               const int col,
                               int *min_alive_cells,
                               int alive_count,
                               int *runtime_states,
                               struct _golsat_tree *tree)
{
    // Base case: unsatisfiable
    if (cmergesat_solve(s) != 10) return 0;

    // Base case: cell cannot be retrieved (out of bounds)
    const int lit = golsat_field_get_lit(field, row, col);
    if (lit == field->is_false) return 0;

    // Try to get the best solution with this literal being dead or alive
    struct _golsat_tree *next_branch = _golsat_formula_next_branch(
        field, row, col, alive_count, lit, runtime_states, tree);
    if (next_branch == NULL) return 0;

    const int next_row = (col == field->width - 1) ? row + 1 : row,
              next_col = (col == field->width - 1) ? 0 : col + 1;

    cmergesat_assume(s, next_branch->lit);
    cmergesat_freeze(s, next_branch->lit);
    runtime_states[row * field->width + col] = next_branch->lit;
    if (_golsat_formula_minimize_alive(
            s, field, next_row, next_col, min_alive_cells,
            alive_count + (next_branch->lit > 0), runtime_states, next_branch))
    {
        _golsat_tree_mark_visited(next_branch);
        return 1;
    }
    runtime_states[row * field->width + col] = 0;
    cmergesat_melt(s, next_branch->lit);

    // Base case: minimized solution doesn't improve current best
    //  (SAT but ignore)
    const int current_alive_cells = golsat_field_count_true_lit(s, field);
    if (current_alive_cells >= *min_alive_cells) return 0;

    *min_alive_cells = current_alive_cells;

    return 1;
}

int
golsat_formula_minimize_alive(CMergeSat *s, struct golsat_field *field)
{
    const int size = field->width * field->height;
    int *best_states = (int *)malloc(size * sizeof(int)),
        *runtime_states = (int *)calloc(size, sizeof(int));
    int retval;

    if (!best_states || !runtime_states) {
        free(best_states);
        free(runtime_states);
        return 0;
    }
    if ((retval = cmergesat_solve(s)) != 10) {
        printf("ERROR: initial formula is unsatisfiable (%d)\n", retval);
        return retval;
    }

    int min_alive_cells = golsat_field_count_true_lit(s, field);
    int best_min_alive = min_alive_cells;
    const double max_execution_time = 240.0; // 4 minutes in seconds
    double total_execution_time = 0.0, last_execution_time = 0;

    for (int i = 0; i < size; ++i)
        best_states[i] = cmergesat_val(s, i);

    struct _golsat_tree *root = _golsat_tree_next(NULL, 0);
    do {
        const clock_t start_time = clock();
        retval = _golsat_formula_minimize_alive(
            s, field, 0, 0, &min_alive_cells, 0, runtime_states, root);
        const double execution_time =
            ((double)(clock() - start_time)) / CLOCKS_PER_SEC;

        total_execution_time += execution_time;

        // Check if we hit the time limit
        if (total_execution_time >= max_execution_time) {
            printf("-- Time limit reached (%.2f seconds)\n",
                   total_execution_time);
            break;
        }

        // Check if we got a better solution
        if (min_alive_cells < best_min_alive) {
            best_min_alive = min_alive_cells;
        }

        const char *status;
        if (retval == 1) {
            status = "CHANGE";
            for (int i = 0; i < size; ++i)
                best_states[i] = cmergesat_val(s, i);
            printf("-- (%s) Minimized to %d alive cells\n", status,
                   min_alive_cells);
            printf("   Execution time: %lf seconds\n",
                   total_execution_time - last_execution_time);
        }
        else {
            status = "CONTINUE";
        }
    } while (root->left->visited != 1 && root->right->visited != 1);

    printf("-- Total execution time: %lf seconds\n", total_execution_time);
    printf("-- Best solution found: %d alive cells\n\n", best_min_alive);

    // Reapply best solution found
    for (int i = 0; i < size; ++i)
        cmergesat_assume(s, best_states[i]);

    free(best_states);
    free(runtime_states);
    _golsat_tree_cleanup(root);

    return cmergesat_solve(s);
}
