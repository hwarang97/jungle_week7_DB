#include <assert.h>
#include <stdio.h>

#include "../src/bptree.h"

static void test_create(void) {
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(tree->root != NULL);

    bptree_destroy(tree);
}

static void test_insert(void) {
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(bptree_insert(tree, 1, tree) == 0);

    bptree_destroy(tree);
}

static void test_search(void) {
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(bptree_insert(tree, 7, tree) == 0);
    assert(bptree_search(tree, 7) == tree);
    assert(bptree_search(tree, 9) == NULL);

    bptree_destroy(tree);
}

static void test_split(void) {
    /* TODO: 사이클 2에서 분할 테스트를 구체화한다. */
}

int main(void) {
    test_create();
    test_insert();
    test_search();
    test_split();

    printf("All tests passed!\n");
    return 0;
}
