#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pattern.h"

struct golsat_pattern *
golsat_pattern_create(FILE *file, int border_disable)
{
    struct golsat_pattern *pattern =
        (struct golsat_pattern *)calloc(1, sizeof *pattern);
    int c, size = 0;
    int capacity;
    int i, j;

    if (!pattern) return NULL;

    if (fscanf(file, "%d %d", &pattern->width, &pattern->height) != 2
        || pattern->width <= 0 || pattern->height <= 0)
    {
        fprintf(stderr,
                "Pattern parsing failed when reading WIDTH and HEIGHT.\n");
        return NULL;
    }
    capacity = pattern->width * pattern->height;

    pattern->cells = (enum golsat_cellstate *)malloc(
        capacity * sizeof(enum golsat_cellstate));
    if (pattern->cells == NULL) {
        free(pattern);
        return NULL;
    }

    /* CNV format */
    while ((c = fgetc(file)) != EOF) {
        enum golsat_cellstate cell;
        switch (c) {
        case '.':
        case '0':
            cell = GOLSAT_CELLSTATE_DEAD;
            break;
        case 'X':
        case '1':
            cell = GOLSAT_CELLSTATE_ALIVE;
            break;
        case '?':
            cell = GOLSAT_CELLSTATE_UNKNOWN;
            break;
        default:
            continue;
        }
        if (size < capacity) {
            pattern->cells[size++] = cell;
            continue;
        }

        fprintf(stderr, "Pattern parsing failed when parsing cells "
                        "(too many characters).\n");
        golsat_pattern_cleanup(pattern);
        return NULL;
    }

    /* check if border exists */
    if (!border_disable) {
        for (i = 0; i < pattern->height; ++i) {
            if (pattern->cells[pattern->width * i] != GOLSAT_CELLSTATE_DEAD
                || pattern->cells[(pattern->width - 1) + pattern->width * i]
                       != GOLSAT_CELLSTATE_DEAD)
            {
                border_disable = 1;
                break;
            }
        }
        for (j = border_disable ? pattern->width : 0; j < pattern->width; ++j)
        {
            if (pattern->cells[j] != GOLSAT_CELLSTATE_DEAD
                || pattern->cells[j + pattern->width * (pattern->height - 1)]
                       != GOLSAT_CELLSTATE_DEAD)
            {
                border_disable = 1;
                break;
            }
        }
    }

    if (!border_disable) {
        /* Fill the border with fixed dead cells. */
        for (i = 0; i < pattern->height; ++i) {
            pattern->cells[pattern->width * i] = GOLSAT_CELLSTATE_FIXED_DEAD;
            pattern->cells[(pattern->width - 1) + pattern->width * i] =
                GOLSAT_CELLSTATE_FIXED_DEAD;
        }
        for (j = 0; j < pattern->width; ++j) {
            pattern->cells[j] = GOLSAT_CELLSTATE_FIXED_DEAD;
            pattern->cells[j + pattern->width * (pattern->height - 1)] =
                GOLSAT_CELLSTATE_FIXED_DEAD;
        }
    }

    if (size != capacity) {
        fprintf(stderr, "Pattern parsing failed when parsing cell (not enough "
                        "characters).\n");
        golsat_pattern_cleanup(pattern);
        return NULL;
    }

    return pattern;
}

void
golsat_pattern_cleanup(struct golsat_pattern *pattern)
{
    free(pattern->cells);
    free(pattern);
}

enum golsat_cellstate
golsat_pattern_get_cell(const struct golsat_pattern *pattern, int x, int y)
{
    assert(x >= 0 && x < pattern->width && y >= 0 && y < pattern->height);
    return pattern->cells[x + pattern->width * y];
}
