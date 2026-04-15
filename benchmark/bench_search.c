#include <stdio.h>
#include <time.h>

#include "../src/bptree.h"

static void benchmark_bptree_search(void) {
    clock_t start = clock();
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    /* TODO: 수렴 Phase에서 실제 검색 벤치마크를 넣는다. */
    printf("B+ Tree search benchmark: %.6f sec\n", elapsed);
}

static void benchmark_linear_search(void) {
    clock_t start = clock();
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Linear search benchmark: %.6f sec\n", elapsed);
}

int main(void) {
    benchmark_bptree_search();
    benchmark_linear_search();
    return 0;
}
