#include <assert.h>
#include <locale.h>
#include <stdio.h>

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

static BPTree *build_two_leaf_tree(int left_keys[2], void *left_values[2],
                                   int right_keys[2], void *right_values[2])
{
    BPTree *tree = bptree_create();
    BPTreeNode *left_leaf;
    BPTreeNode *right_leaf;

    assert(tree != NULL);

    left_leaf = bptree_create_node(1);
    right_leaf = bptree_create_node(1);

    assert(left_leaf != NULL);
    assert(right_leaf != NULL);

    tree->root->is_leaf = 0;
    tree->root->key_count = 1;
    tree->root->keys[0] = right_keys[0];
    tree->root->children[0] = left_leaf;
    tree->root->children[1] = right_leaf;

    left_leaf->parent = tree->root;
    right_leaf->parent = tree->root;
    left_leaf->next = right_leaf;
    tree->first_leaf = left_leaf;

    left_leaf->key_count = 2;
    left_leaf->keys[0] = left_keys[0];
    left_leaf->keys[1] = left_keys[1];
    left_leaf->values[0] = left_values[0];
    left_leaf->values[1] = left_values[1];

    right_leaf->key_count = 2;
    right_leaf->keys[0] = right_keys[0];
    right_leaf->keys[1] = right_keys[1];
    right_leaf->values[0] = right_values[0];
    right_leaf->values[1] = right_values[1];

    return tree;
}

static BPTree *build_three_leaf_tree(void)
{
    BPTree *tree = bptree_create();
    BPTreeNode *left_leaf;
    BPTreeNode *middle_leaf;
    BPTreeNode *right_leaf;
    static int v10 = 10;
    static int v20 = 20;
    static int v30 = 30;
    static int v40 = 40;
    static int v50 = 50;
    static int v60 = 60;

    assert(tree != NULL);

    left_leaf = bptree_create_node(1);
    middle_leaf = bptree_create_node(1);
    right_leaf = bptree_create_node(1);

    assert(left_leaf != NULL);
    assert(middle_leaf != NULL);
    assert(right_leaf != NULL);

    tree->root->is_leaf = 0;
    tree->root->key_count = 2;
    tree->root->keys[0] = 30;
    tree->root->keys[1] = 50;
    tree->root->children[0] = left_leaf;
    tree->root->children[1] = middle_leaf;
    tree->root->children[2] = right_leaf;

    left_leaf->parent = tree->root;
    middle_leaf->parent = tree->root;
    right_leaf->parent = tree->root;

    left_leaf->next = middle_leaf;
    middle_leaf->next = right_leaf;
    tree->first_leaf = left_leaf;

    left_leaf->key_count = 2;
    left_leaf->keys[0] = 10;
    left_leaf->keys[1] = 20;
    left_leaf->values[0] = &v10;
    left_leaf->values[1] = &v20;

    middle_leaf->key_count = 2;
    middle_leaf->keys[0] = 30;
    middle_leaf->keys[1] = 40;
    middle_leaf->values[0] = &v30;
    middle_leaf->values[1] = &v40;

    right_leaf->key_count = 2;
    right_leaf->keys[0] = 50;
    right_leaf->keys[1] = 60;
    right_leaf->values[0] = &v50;
    right_leaf->values[1] = &v60;

    return tree;
}

static void test_search_basic_two_leaf(void)
{
    static int left_value = 100;
    static int left_second_value = 110;
    static int right_value = 200;
    static int right_second_value = 210;
    int left_keys[2] = {10, 20};
    int right_keys[2] = {30, 40};
    void *left_values[2] = {&left_value, &left_second_value};
    void *right_values[2] = {&right_value, &right_second_value};
    BPTree *tree = build_two_leaf_tree(left_keys, left_values, right_keys, right_values);

    assert(bptree_search(tree, 10) == &left_value);
    assert(bptree_search(tree, 30) == &right_value);
    assert(bptree_search(tree, 25) == NULL);

    bptree_destroy(tree);
}

static void test_search_empty_tree_returns_null(void)
{
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(bptree_search(tree, 999) == NULL);

    bptree_destroy(tree);
}

static void test_search_single_leaf_first_key(void)
{
    BPTree *tree = bptree_create();
    static int value = 101;

    tree->root->key_count = 1;
    tree->root->keys[0] = 7;
    tree->root->values[0] = &value;

    assert(bptree_search(tree, 7) == &value);

    bptree_destroy(tree);
}

static void test_search_single_leaf_last_key(void)
{
    BPTree *tree = bptree_create();
    static int first = 1;
    static int second = 2;
    static int third = 3;
    int key_count = BPTREE_MAX_KEYS < 3 ? BPTREE_MAX_KEYS : 3;

    tree->root->key_count = key_count;
    tree->root->keys[0] = 5;
    tree->root->values[0] = &first;
    if (key_count >= 2) {
        tree->root->keys[1] = 15;
        tree->root->values[1] = &second;
    }
#if BPTREE_MAX_KEYS >= 3
    if (key_count >= 3) {
        tree->root->keys[2] = 25;
        tree->root->values[2] = &third;
    }
#endif

    if (key_count == 3) {
        assert(bptree_search(tree, 25) == &third);
    } else {
        assert(bptree_search(tree, 15) == &second);
    }

    bptree_destroy(tree);
}

static void test_search_single_leaf_missing_key_between_values(void)
{
    BPTree *tree = bptree_create();
    static int first = 1;
    static int second = 2;

    tree->root->key_count = 2;
    tree->root->keys[0] = 10;
    tree->root->keys[1] = 20;
    tree->root->values[0] = &first;
    tree->root->values[1] = &second;

    assert(bptree_search(tree, 15) == NULL);

    bptree_destroy(tree);
}

static void test_search_two_leaf_left_boundary_key(void)
{
    static int left_value = 100;
    static int left_second_value = 110;
    static int right_value = 200;
    static int right_second_value = 210;
    int left_keys[2] = {10, 20};
    int right_keys[2] = {30, 40};
    void *left_values[2] = {&left_value, &left_second_value};
    void *right_values[2] = {&right_value, &right_second_value};
    BPTree *tree = build_two_leaf_tree(left_keys, left_values, right_keys, right_values);

    assert(bptree_search(tree, 20) == &left_second_value);

    bptree_destroy(tree);
}

static void test_search_two_leaf_separator_key_goes_right(void)
{
    static int left_value = 100;
    static int left_second_value = 110;
    static int right_value = 200;
    static int right_second_value = 210;
    int left_keys[2] = {10, 20};
    int right_keys[2] = {30, 40};
    void *left_values[2] = {&left_value, &left_second_value};
    void *right_values[2] = {&right_value, &right_second_value};
    BPTree *tree = build_two_leaf_tree(left_keys, left_values, right_keys, right_values);

    assert(bptree_search(tree, 30) == &right_value);

    bptree_destroy(tree);
}

static void test_search_two_leaf_missing_key_smaller_than_all(void)
{
    static int left_value = 100;
    static int left_second_value = 110;
    static int right_value = 200;
    static int right_second_value = 210;
    int left_keys[2] = {10, 20};
    int right_keys[2] = {30, 40};
    void *left_values[2] = {&left_value, &left_second_value};
    void *right_values[2] = {&right_value, &right_second_value};
    BPTree *tree = build_two_leaf_tree(left_keys, left_values, right_keys, right_values);

    assert(bptree_search(tree, 1) == NULL);

    bptree_destroy(tree);
}

static void test_search_two_leaf_missing_key_larger_than_all(void)
{
    static int left_value = 100;
    static int left_second_value = 110;
    static int right_value = 200;
    static int right_second_value = 210;
    int left_keys[2] = {10, 20};
    int right_keys[2] = {30, 40};
    void *left_values[2] = {&left_value, &left_second_value};
    void *right_values[2] = {&right_value, &right_second_value};
    BPTree *tree = build_two_leaf_tree(left_keys, left_values, right_keys, right_values);

    assert(bptree_search(tree, 99) == NULL);

    bptree_destroy(tree);
}

static void test_search_three_leaf_middle_branch(void)
{
    BPTree *tree = build_three_leaf_tree();
    int *result = (int *)bptree_search(tree, 40);

    assert(result != NULL);
    assert(*result == 40);

    bptree_destroy(tree);
}

static void test_search_three_leaf_rightmost_branch(void)
{
    BPTree *tree = build_three_leaf_tree();
    int *result = (int *)bptree_search(tree, 60);

    assert(result != NULL);
    assert(*result == 60);

    bptree_destroy(tree);
}

static void run_test(const TestCase *test_case, int index, int total)
{
    printf("[SEARCH %d/%d] %s ... ", index, total, test_case->name);
    fflush(stdout);
    test_case->fn();
    printf("PASS\n");
}

int main(void)
{
    TestCase tests[] = {
        {"basic: 내부 노드를 따라 내려가 리프에서 key 를 찾는다", test_search_basic_two_leaf},
        {"case-01: 빈 트리에서는 어떤 key 도 찾지 못한다", test_search_empty_tree_returns_null},
        {"case-02: 리프 하나만 있을 때 첫 번째 key 를 찾는다", test_search_single_leaf_first_key},
        {"case-03: 리프 하나만 있을 때 마지막 key 를 찾는다", test_search_single_leaf_last_key},
        {"case-04: 리프 안에 없는 중간 key 는 NULL 이다", test_search_single_leaf_missing_key_between_values},
        {"case-05: 왼쪽 리프의 경계 key 를 찾는다", test_search_two_leaf_left_boundary_key},
        {"case-06: 분기 기준 key 는 오른쪽 리프로 내려간다", test_search_two_leaf_separator_key_goes_right},
        {"case-07: 전체 최소값보다 작은 key 는 NULL 이다", test_search_two_leaf_missing_key_smaller_than_all},
        {"case-08: 전체 최대값보다 큰 key 는 NULL 이다", test_search_two_leaf_missing_key_larger_than_all},
        {"case-09: 세 개의 리프 중 가운데 리프를 선택한다", test_search_three_leaf_middle_branch},
        {"case-10: 가장 오른쪽 리프까지 내려가 key 를 찾는다", test_search_three_leaf_rightmost_branch},
    };
    int total = (int)(sizeof(tests) / sizeof(tests[0]));
    int i;

    configure_console_utf8();

    for (i = 0; i < total; i++) {
        run_test(&tests[i], i + 1, total);
    }

    printf("Search tests passed! (%d/%d)\n", total, total);
    return 0;
}
