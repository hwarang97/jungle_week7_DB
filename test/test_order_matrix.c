#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/bptree.h"

static int tree_height(BPTree *tree)
{
    BPTreeNode *node;
    int height = 0;

    if (tree == NULL || tree->root == NULL) {
        return 0;
    }

    node = tree->root;
    while (node != NULL) {
        height++;
        if (node->is_leaf) {
            break;
        }
        node = node->children[0];
    }

    return height;
}

int main(void)
{
    const int insert_count = BPTREE_ORDER * BPTREE_ORDER * 2;
    BPTree *tree = bptree_create();
    int *values;
    BPTreeNode *leaf;
    int expected_key;
    int index;

    assert(tree != NULL);

    values = (int *)malloc((size_t)insert_count * sizeof(*values));
    assert(values != NULL);

    for (index = 0; index < insert_count; ++index) {
        values[index] = index + 1;
        assert(bptree_insert(tree, index + 1, &values[index]) == 0);
    }

    assert(tree->root != NULL);
    assert(tree->root->is_leaf == 0);
    assert(tree_height(tree) >= 3);

    for (index = 0; index < insert_count; ++index) {
        assert(bptree_search(tree, index + 1) == &values[index]);
    }

    expected_key = 1;
    leaf = tree->first_leaf;
    while (leaf != NULL) {
        int key_index;

        for (key_index = 0; key_index < leaf->key_count; ++key_index) {
            assert(leaf->keys[key_index] == expected_key);
            expected_key++;
        }
        leaf = leaf->next;
    }

    assert(expected_key == insert_count + 1);

    printf("Order-matrix smoke passed! order=%d inserted=%d height=%d\n",
           BPTREE_ORDER, insert_count, tree_height(tree));

    bptree_destroy(tree);
    free(values);
    return 0;
}
