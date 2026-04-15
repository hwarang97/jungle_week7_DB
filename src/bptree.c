#include "bptree.h"

#include <stdlib.h>

static BPTreeNode *bptree_create_leaf(void) {
    BPTreeNode *node = calloc(1, sizeof(BPTreeNode));
    if (!node) {
        return NULL;
    }

    node->is_leaf = 1;
    return node;
}

static void bptree_destroy_node(BPTreeNode *node) {
    size_t index;

    if (!node) {
        return;
    }

    if (!node->is_leaf) {
        for (index = 0; index <= node->key_count; ++index) {
            bptree_destroy_node(node->children[index]);
        }
    }

    free(node);
}

BPTree *bptree_create(void) {
    BPTree *tree = calloc(1, sizeof(BPTree));
    if (!tree) {
        return NULL;
    }

    tree->root = bptree_create_leaf();
    if (!tree->root) {
        free(tree);
        return NULL;
    }

    return tree;
}

void bptree_destroy(BPTree *tree) {
    if (!tree) {
        return;
    }

    bptree_destroy_node(tree->root);
    free(tree);
}

int bptree_insert(BPTree *tree, int key, void *value) {
    BPTreeNode *leaf;
    size_t index;
    size_t move;

    if (!tree || !tree->root) {
        return -1;
    }

    leaf = tree->root;

    /* TODO: 사이클 1~2에서 리프 탐색과 분할 로직을 구현한다. */
    if (leaf->key_count >= BPTREE_MAX_KEYS) {
        return -1;
    }

    index = 0;
    while (index < leaf->key_count && leaf->keys[index] < key) {
        ++index;
    }

    if (index < leaf->key_count && leaf->keys[index] == key) {
        leaf->values[index] = value;
        return 0;
    }

    move = leaf->key_count;
    while (move > index) {
        leaf->keys[move] = leaf->keys[move - 1];
        leaf->values[move] = leaf->values[move - 1];
        --move;
    }

    leaf->keys[index] = key;
    leaf->values[index] = value;
    ++leaf->key_count;

    return 0;
}

void *bptree_search(const BPTree *tree, int key) {
    BPTreeNode *leaf;
    size_t index;

    if (!tree || !tree->root) {
        return NULL;
    }

    leaf = tree->root;

    /* TODO: 사이클 1에서 내부 노드 탐색 경로를 구현한다. */
    for (index = 0; index < leaf->key_count; ++index) {
        if (leaf->keys[index] == key) {
            return leaf->values[index];
        }
    }

    return NULL;
}
