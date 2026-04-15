#ifndef BPTREE_H
#define BPTREE_H

#include <stddef.h>

#define BPTREE_ORDER 4
#define BPTREE_MAX_KEYS (BPTREE_ORDER - 1)
#define BPTREE_MAX_CHILDREN BPTREE_ORDER

typedef struct BPTreeNode {
    int is_leaf;
    size_t key_count;
    int keys[BPTREE_MAX_KEYS];
    void *values[BPTREE_MAX_KEYS];
    struct BPTreeNode *children[BPTREE_MAX_CHILDREN];
    struct BPTreeNode *next;
} BPTreeNode;

typedef struct BPTree {
    BPTreeNode *root;
} BPTree;

/* TODO: 정렬 Phase에서 구조체와 함수 선언을 더 구체화한다. */
BPTree *bptree_create(void);
void bptree_destroy(BPTree *tree);
int bptree_insert(BPTree *tree, int key, void *value);
void *bptree_search(const BPTree *tree, int key);

#endif
