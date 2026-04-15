#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../src/bptree.h"

#define SEARCH_SAMPLES 10000

static const char *ansi_reset(void)
{
    return "\033[0m";
}

static const char *ansi_bold(void)
{
    return "\033[1m";
}

static const char *ansi_cyan(void)
{
    return "\033[36m";
}

static const char *ansi_green(void)
{
    return "\033[32m";
}

static const char *ansi_yellow(void)
{
    return "\033[33m";
}

static double elapsed_ms(clock_t start, clock_t end)
{
    return ((double)(end - start) * 1000.0) / (double)CLOCKS_PER_SEC;
}

static double bench_bptree_insert(int row_count, int *values)
{
    BPTree *tree;
    clock_t start;
    clock_t end;
    int i;

    tree = bptree_create();
    if (tree == NULL) {
        return -1.0;
    }

    start = clock();
    for (i = 0; i < row_count; ++i) {
        if (bptree_insert(tree, i + 1, &values[i]) != 0) {
            bptree_destroy(tree);
            return -1.0;
        }
    }
    end = clock();

    bptree_destroy(tree);
    return elapsed_ms(start, end);
}

static double bench_bptree_search(int row_count, int *values)
{
    BPTree *tree;
    clock_t start;
    clock_t end;
    int i;

    tree = bptree_create();
    if (tree == NULL) {
        return -1.0;
    }

    for (i = 0; i < row_count; ++i) {
        if (bptree_insert(tree, i + 1, &values[i]) != 0) {
            bptree_destroy(tree);
            return -1.0;
        }
    }

    start = clock();
    for (i = 0; i < SEARCH_SAMPLES; ++i) {
        int target = ((i * 97) % row_count) + 1;
        if (bptree_search(tree, target) == NULL) {
            bptree_destroy(tree);
            return -1.0;
        }
    }
    end = clock();

    bptree_destroy(tree);
    return elapsed_ms(start, end);
}

static double bench_linear_search(int row_count, int *values)
{
    clock_t start;
    clock_t end;
    int i;

    start = clock();
    for (i = 0; i < SEARCH_SAMPLES; ++i) {
        int target = ((i * 97) % row_count) + 1;
        int index;
        int found = 0;

        for (index = 0; index < row_count; ++index) {
            if (values[index] == target) {
                found = 1;
                break;
            }
        }

        if (!found) {
            return -1.0;
        }
    }
    end = clock();

    return elapsed_ms(start, end);
}

static void run_case(int row_count)
{
    int *values;
    double insert_ms;
    double bptree_search_ms;
    double linear_search_ms;
    int i;

    values = (int *)malloc((size_t)row_count * sizeof(*values));
    if (values == NULL) {
        printf("[bench-bptree-scale] allocation failed for rows=%d\n", row_count);
        return;
    }

    for (i = 0; i < row_count; ++i) {
        values[i] = i + 1;
    }

    insert_ms = bench_bptree_insert(row_count, values);
    bptree_search_ms = bench_bptree_search(row_count, values);
    linear_search_ms = bench_linear_search(row_count, values);

    printf("\n%s%s[CASE]%s rows=%s%d%s order=%s%d%s\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_bold(), row_count, ansi_reset(),
           ansi_bold(), BPTREE_ORDER, ansi_reset());
    printf("  %s[bptree insert]%s total=%s%.3fms%s avg=%s%.6fms%s\n",
           ansi_green(), ansi_reset(),
           ansi_bold(), insert_ms, ansi_reset(),
           ansi_bold(), insert_ms / (double)row_count, ansi_reset());
    printf("  %s[bptree search]%s total=%s%.3fms%s avg=%s%.6fms%s\n",
           ansi_green(), ansi_reset(),
           ansi_bold(), bptree_search_ms, ansi_reset(),
           ansi_bold(), bptree_search_ms / (double)SEARCH_SAMPLES, ansi_reset());
    printf("  %s[linear search]%s total=%s%.3fms%s avg=%s%.6fms%s\n",
           ansi_yellow(), ansi_reset(),
           ansi_bold(), linear_search_ms, ansi_reset(),
           ansi_bold(), linear_search_ms / (double)SEARCH_SAMPLES, ansi_reset());

    free(values);
}

int main(void)
{
    int row_counts[] = {10000, 50000, 200000};
    int i;

    printf("%s%s[bench-bptree-scale]%s current_bptree_order=%s%d%s samples=%s%d%s\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_bold(), BPTREE_ORDER, ansi_reset(),
           ansi_bold(), SEARCH_SAMPLES, ansi_reset());
    printf("%s%s[goal]%s sequential insert + repeated key lookup vs linear search\n",
           ansi_bold(), ansi_cyan(), ansi_reset());

    for (i = 0; i < (int)(sizeof(row_counts) / sizeof(row_counts[0])); ++i) {
        run_case(row_counts[i]);
    }

    return 0;
}
