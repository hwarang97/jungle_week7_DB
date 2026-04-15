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

static void test_split_default_state(void)
{
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(tree->root->next == NULL);

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
        {"create: 빈 트리를 만들면 루트 리프가 준비된다", test_create},
        {"split: 현재 골격에서 리프 연결 포인터 기본값은 NULL 이다", test_split_default_state},
    };
    int total = (int)(sizeof(tests) / sizeof(tests[0]));
    int i;

    for (i = 0; i < total; i++) {
        run_test(&tests[i], i + 1, total);
    }

    printf("Structure tests passed! (%d/%d)\n", total, total);
    return 0;
}
