#include <stdio.h>
#include <time.h>

#include "../src/bptree.h"

static void benchmark_bptree_search(void)
{
    clock_t start = clock();
    BPTree *tree = bptree_create();
    clock_t end;

    if (!tree) {
        printf("B+ Tree benchmark setup failed\n");
        return;
    }

    (void)bptree_search(tree, 1000000);
    end = clock();

    printf("B+ Tree search skeleton: %.6f sec\n",
           (double)(end - start) / CLOCKS_PER_SEC);
    bptree_destroy(tree);
}

static void benchmark_linear_search(void)
{
    clock_t start = clock();
    volatile int found = 0;
    clock_t end;

    (void)found;
    end = clock();

    printf("Linear search skeleton: %.6f sec\n",
           (double)(end - start) / CLOCKS_PER_SEC);
}

int main(void)
{
    benchmark_bptree_search();
    benchmark_linear_search();
    return 0;
}
