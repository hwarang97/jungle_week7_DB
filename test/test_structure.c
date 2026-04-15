#include <assert.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/bptree.h"

#ifdef _WIN32
#include <windows.h>
#endif

typedef void (*TestFn)(void);

typedef struct TestCase {
    const char *name;
    TestFn fn;
} TestCase;

static void configure_console_utf8(void)
{
    setlocale(LC_ALL, ".UTF-8");

#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

static void fill_identity_order(int *order, int key_count)
{
    int index;

    for (index = 0; index < key_count; index++) {
        order[index] = index;
    }
}

static void reverse_order(int *order, int key_count)
{
    int left = 0;
    int right = key_count - 1;

    while (left < right) {
        int temp = order[left];
        order[left] = order[right];
        order[right] = temp;
        left++;
        right--;
    }
}

static void fill_even_then_odd_order(int *order, int key_count)
{
    int write_index = 0;
    int index;

    for (index = 0; index < key_count; index += 2) {
        order[write_index++] = index;
    }
    for (index = 1; index < key_count; index += 2) {
        order[write_index++] = index;
    }
}

static void shuffle_order_with_seed(int *order, int key_count, unsigned int seed)
{
    int index;

    srand(seed);
    for (index = key_count - 1; index > 0; index--) {
        int swap_index = rand() % (index + 1);
        int temp = order[index];
        order[index] = order[swap_index];
        order[swap_index] = temp;
    }
}

static const BPTreeNode *leftmost_leaf(const BPTreeNode *node)
{
    assert(node != NULL);

    while (!node->is_leaf) {
        assert(node->children[0] != NULL);
        node = node->children[0];
    }

    return node;
}

static int subtree_first_key(const BPTreeNode *node)
{
    const BPTreeNode *leaf = leftmost_leaf(node);

    assert(leaf->key_count > 0);
    return leaf->keys[0];
}

static void assert_node_keys_sorted(const BPTreeNode *node)
{
    int index;

    for (index = 1; index < node->key_count; index++) {
        assert(node->keys[index - 1] < node->keys[index]);
    }
}

static int assert_subtree_invariants(const BPTreeNode *node,
                                     const BPTreeNode *parent,
                                     int depth,
                                     int *leaf_depth,
                                     int *leaf_count)
{
    int total_keys = 0;
    int index;

    assert(node != NULL);
    assert(node->parent == parent);
    assert(node->key_count >= 0);
    assert(node->key_count <= BPTREE_MAX_KEYS);
    assert_node_keys_sorted(node);

    if (node->is_leaf) {
        if (*leaf_depth < 0) {
            *leaf_depth = depth;
        } else {
            assert(*leaf_depth == depth);
        }
        (*leaf_count)++;
        return node->key_count;
    }

    assert(node->key_count > 0);
    for (index = 0; index <= node->key_count; index++) {
        assert(node->children[index] != NULL);
    }
    for (index = node->key_count + 1; index < BPTREE_MAX_CHILDREN; index++) {
        assert(node->children[index] == NULL);
    }
    for (index = 0; index < node->key_count; index++) {
        assert(node->keys[index] == subtree_first_key(node->children[index + 1]));
    }

    for (index = 0; index <= node->key_count; index++) {
        total_keys += assert_subtree_invariants(node->children[index],
                                                node,
                                                depth + 1,
                                                leaf_depth,
                                                leaf_count);
    }

    return total_keys;
}

static void assert_leaf_chain_invariants(const BPTree *tree,
                                         int expected_key_count,
                                         int expected_leaf_count)
{
    const BPTreeNode *leaf;
    int previous_key = INT_MIN;
    int has_previous = 0;
    int seen_keys = 0;
    int seen_leaves = 0;

    for (leaf = tree->first_leaf; leaf != NULL; leaf = leaf->next) {
        int index;

        assert(leaf->is_leaf == 1);
        assert_node_keys_sorted(leaf);
        seen_leaves++;

        for (index = 0; index < leaf->key_count; index++) {
            if (has_previous) {
                assert(previous_key < leaf->keys[index]);
            }
            previous_key = leaf->keys[index];
            has_previous = 1;
            seen_keys++;
        }
    }

    assert(seen_keys == expected_key_count);
    assert(seen_leaves == expected_leaf_count);
}

static void assert_tree_invariants(const BPTree *tree, int expected_key_count)
{
    int leaf_depth = -1;
    int leaf_count = 0;
    int total_keys;

    assert(tree != NULL);
    assert(tree->root != NULL);
    assert(tree->root->parent == NULL);
    assert(tree->first_leaf == leftmost_leaf(tree->root));

    total_keys = assert_subtree_invariants(tree->root, NULL, 0, &leaf_depth, &leaf_count);
    assert(total_keys == expected_key_count);
    assert_leaf_chain_invariants(tree, expected_key_count, leaf_count);
}

static void test_create(void)
{
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(tree->root != NULL);
    assert(tree->root->is_leaf == 1);
    assert(tree->root->key_count == 0);
    assert(tree->first_leaf == tree->root);
    assert(tree->root->parent == NULL);
    assert_tree_invariants(tree, 0);

    bptree_destroy(tree);
}

static void test_insert_single_leaf_sorted(void)
{
    BPTree *tree = bptree_create();
    int values[3] = {200, 100, 300};

    assert(tree != NULL);
    assert(bptree_insert(tree, 20, &values[0]) == 0);
    assert(bptree_insert(tree, 10, &values[1]) == 0);
    assert(bptree_insert(tree, 30, &values[2]) == 0);

    assert(tree->root->is_leaf == 1);
    assert(tree->root->key_count == 3);
    assert(tree->root->keys[0] == 10);
    assert(tree->root->keys[1] == 20);
    assert(tree->root->keys[2] == 30);
    assert(bptree_search(tree, 10) == &values[1]);
    assert(bptree_search(tree, 20) == &values[0]);
    assert(bptree_search(tree, 30) == &values[2]);
    assert_tree_invariants(tree, 3);

    bptree_destroy(tree);
}

static void test_duplicate_key_updates_value(void)
{
    BPTree *tree = bptree_create();
    int old_value = 10;
    int new_value = 99;

    assert(tree != NULL);
    assert(bptree_insert(tree, 7, &old_value) == 0);
    assert(bptree_insert(tree, 7, &new_value) == 0);

    assert(tree->root->is_leaf == 1);
    assert(tree->root->key_count == 1);
    assert(tree->root->keys[0] == 7);
    assert(bptree_search(tree, 7) == &new_value);
    assert_tree_invariants(tree, 1);

    bptree_destroy(tree);
}

static void test_leaf_split_creates_new_root(void)
{
    BPTree *tree = bptree_create();
    int values[4] = {10, 20, 30, 40};
    BPTreeNode *left_leaf;
    BPTreeNode *right_leaf;

    assert(tree != NULL);
    assert(bptree_insert(tree, 10, &values[0]) == 0);
    assert(bptree_insert(tree, 20, &values[1]) == 0);
    assert(bptree_insert(tree, 30, &values[2]) == 0);
    assert(bptree_insert(tree, 40, &values[3]) == 0);

    assert(tree->root->is_leaf == 0);
    assert(tree->root->key_count == 1);
    assert(tree->root->keys[0] == 30);

    left_leaf = tree->root->children[0];
    right_leaf = tree->root->children[1];

    assert(left_leaf != NULL);
    assert(right_leaf != NULL);
    assert(left_leaf->is_leaf == 1);
    assert(right_leaf->is_leaf == 1);
    assert(left_leaf->key_count == 2);
    assert(right_leaf->key_count == 2);
    assert(left_leaf->keys[0] == 10);
    assert(left_leaf->keys[1] == 20);
    assert(right_leaf->keys[0] == 30);
    assert(right_leaf->keys[1] == 40);
    assert(left_leaf->next == right_leaf);
    assert(tree->first_leaf == left_leaf);
    assert_tree_invariants(tree, 4);

    bptree_destroy(tree);
}

static void test_internal_split_keeps_search_and_leaf_chain(void)
{
    BPTree *tree = bptree_create();
    int values[10];
    BPTreeNode *left_internal;
    BPTreeNode *right_internal;
    BPTreeNode *leaf;
    int expected_first_keys[5] = {10, 30, 50, 70, 90};
    int index;

    assert(tree != NULL);

    for (index = 0; index < 10; index++) {
        values[index] = (index + 1) * 10;
        assert(bptree_insert(tree, (index + 1) * 10, &values[index]) == 0);
    }

    assert(tree->root->is_leaf == 0);
    assert(tree->root->key_count == 1);
    assert(tree->root->keys[0] == 70);

    left_internal = tree->root->children[0];
    right_internal = tree->root->children[1];

    assert(left_internal != NULL);
    assert(right_internal != NULL);
    assert(left_internal->is_leaf == 0);
    assert(right_internal->is_leaf == 0);
    assert(left_internal->key_count == 2);
    assert(right_internal->key_count == 1);
    assert(left_internal->keys[0] == 30);
    assert(left_internal->keys[1] == 50);
    assert(right_internal->keys[0] == 90);

    leaf = tree->first_leaf;
    for (index = 0; index < 5; index++) {
        assert(leaf != NULL);
        assert(leaf->keys[0] == expected_first_keys[index]);
        leaf = leaf->next;
    }
    assert(leaf == NULL);

    for (index = 0; index < 10; index++) {
        assert(bptree_search(tree, (index + 1) * 10) == &values[index]);
    }
    assert(bptree_search(tree, 55) == NULL);
    assert_tree_invariants(tree, 10);

    bptree_destroy(tree);
}

static void test_insert_stress_patterns_keep_invariants(void)
{
    enum { KEY_COUNT = 64 };
    int keys[KEY_COUNT];
    int values[KEY_COUNT];
    int order[KEY_COUNT];
    unsigned int shuffle_seeds[2] = {20260415u, 777u};
    int pattern_index;
    int index;

    for (index = 0; index < KEY_COUNT; index++) {
        keys[index] = (index + 1) * 3;
        values[index] = keys[index] * 10;
    }

    for (pattern_index = 0; pattern_index < 5; pattern_index++) {
        BPTree *tree = bptree_create();

        assert(tree != NULL);
        fill_identity_order(order, KEY_COUNT);

        if (pattern_index == 1) {
            reverse_order(order, KEY_COUNT);
        } else if (pattern_index == 2) {
            fill_even_then_odd_order(order, KEY_COUNT);
        } else if (pattern_index == 3) {
            shuffle_order_with_seed(order, KEY_COUNT, shuffle_seeds[0]);
        } else if (pattern_index == 4) {
            shuffle_order_with_seed(order, KEY_COUNT, shuffle_seeds[1]);
        }

        for (index = 0; index < KEY_COUNT; index++) {
            int key_index = order[index];
            assert(bptree_insert(tree, keys[key_index], &values[key_index]) == 0);
        }

        for (index = 0; index < KEY_COUNT; index++) {
            assert(bptree_search(tree, keys[index]) == &values[index]);
        }
        assert(bptree_search(tree, 1) == NULL);
        assert(bptree_search(tree, 500) == NULL);
        assert_tree_invariants(tree, KEY_COUNT);

        bptree_destroy(tree);
    }
}

static void test_int_max_key_keeps_invariants(void)
{
    BPTree *tree = bptree_create();
    int near_max_value = 111;
    int max_value = 222;

    assert(tree != NULL);
    assert(bptree_insert(tree, INT_MAX - 1, &near_max_value) == 0);
    assert(bptree_insert(tree, INT_MAX, &max_value) == 0);
    assert(bptree_search(tree, INT_MAX - 1) == &near_max_value);
    assert(bptree_search(tree, INT_MAX) == &max_value);
    assert_tree_invariants(tree, 2);

    bptree_destroy(tree);
}

static void run_test(const TestCase *test_case, int index, int total)
{
    printf("[STRUCTURE %d/%d] %s ... ", index, total, test_case->name);
    fflush(stdout);
    test_case->fn();
    printf("PASS\n");
}

int main(void)
{
    TestCase tests[] = {
        {"create starts with a single empty leaf root", test_create},
        {"single-leaf insert keeps keys sorted", test_insert_single_leaf_sorted},
        {"duplicate key updates only the value", test_duplicate_key_updates_value},
        {"leaf split creates a new internal root", test_leaf_split_creates_new_root},
        {"internal split preserves search and leaf chain", test_internal_split_keeps_search_and_leaf_chain},
        {"stress patterns preserve search and invariants", test_insert_stress_patterns_keep_invariants},
        {"int max key keeps search and invariants", test_int_max_key_keeps_invariants},
    };
    int total = (int)(sizeof(tests) / sizeof(tests[0]));
    int i;

    configure_console_utf8();

    for (i = 0; i < total; i++) {
        run_test(&tests[i], i + 1, total);
    }

    printf("Structure tests passed! (%d/%d)\n", total, total);
    return 0;
}
