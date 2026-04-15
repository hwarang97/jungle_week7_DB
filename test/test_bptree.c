#include <assert.h>
#include <stdio.h>

#include "../src/bptree.h"

typedef void (*TestFn)(void);

typedef struct TestCase {
    const char *name;
    TestFn fn;
} TestCase;

static void test_create(void)
{
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(tree->root != NULL);
    assert(tree->root->is_leaf == 1);
    assert(tree->root->key_count == 0);
    assert(tree->first_leaf == tree->root);
    assert(tree->root->parent == NULL);

    bptree_destroy(tree);
}

static void test_insert(void)
{
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(bptree_insert(tree, 10, NULL) == -1);

    bptree_destroy(tree);
}

static void test_search(void)
{
    BPTree *tree = bptree_create();
    BPTreeNode *left_leaf;
    BPTreeNode *right_leaf;
    static int left_value = 100;
    static int right_value = 200;

    assert(tree != NULL);

    left_leaf = bptree_create_node(1);
    right_leaf = bptree_create_node(1);

    assert(left_leaf != NULL);
    assert(right_leaf != NULL);

    tree->root->is_leaf = 0;
    tree->root->key_count = 1;
    tree->root->keys[0] = 30;
    tree->root->children[0] = left_leaf;
    tree->root->children[1] = right_leaf;

    left_leaf->parent = tree->root;
    right_leaf->parent = tree->root;
    left_leaf->next = right_leaf;
    tree->first_leaf = left_leaf;

    left_leaf->key_count = 2;
    left_leaf->keys[0] = 10;
    left_leaf->keys[1] = 20;
    left_leaf->values[0] = &left_value;

    right_leaf->key_count = 2;
    right_leaf->keys[0] = 30;
    right_leaf->keys[1] = 40;
    right_leaf->values[0] = &right_value;

    assert(bptree_search(tree, 10) == &left_value);
    assert(bptree_search(tree, 30) == &right_value);
    assert(bptree_search(tree, 25) == NULL);

    bptree_destroy(tree);
}

static void test_split(void)
{
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(tree->root->next == NULL);

    bptree_destroy(tree);
}

static void run_test(const TestCase *test_case, int index, int total)
{
    printf("[TEST %d/%d] %s ... ", index, total, test_case->name);
    fflush(stdout);
    test_case->fn();
    printf("PASS\n");
}

int main(void)
{
    TestCase tests[] = {
        {"create: 빈 트리를 만들면 루트 리프가 준비된다", test_create},
        {"insert: 아직 미구현이라 -1 을 돌려준다", test_insert},
        {"search: 내부 노드를 따라 내려가 리프에서 key 를 찾는다", test_search},
        {"split: 현재 골격에서 리프 연결 포인터 기본값은 NULL 이다", test_split},
    };
    int total = (int)(sizeof(tests) / sizeof(tests[0]));
    int i;

    for (i = 0; i < total; i++) {
        run_test(&tests[i], i + 1, total);
    }

    printf("All tests passed! (%d/%d)\n", total, total);
    return 0;
}
