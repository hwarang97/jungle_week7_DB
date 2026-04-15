#include <assert.h>
#include <stdio.h>

#include "../src/bptree.h"

typedef void (*TestFn)(void);

typedef struct TestCase {
    const char *name;
    TestFn fn;
} TestCase;

static void test_insert_into_empty_leaf(void)
{
    BPTree *tree = bptree_create();
    static int value = 100;

    assert(tree != NULL);
    assert(bptree_insert(tree, 10, &value) == 0);
    assert(tree->root->key_count == 1);
    assert(tree->root->keys[0] == 10);
    assert(bptree_search(tree, 10) == &value);

    bptree_destroy(tree);
}

static void test_insert_keeps_sorted_order(void)
{
    BPTree *tree = bptree_create();
    static int v30 = 30;
    static int v10 = 10;
    static int v20 = 20;

    assert(tree != NULL);
    assert(bptree_insert(tree, 30, &v30) == 0);
    assert(bptree_insert(tree, 10, &v10) == 0);
    assert(bptree_insert(tree, 20, &v20) == 0);

    assert(tree->root->key_count == 3);
    assert(tree->root->keys[0] == 10);
    assert(tree->root->keys[1] == 20);
    assert(tree->root->keys[2] == 30);
    assert(tree->root->values[0] == &v10);
    assert(tree->root->values[1] == &v20);
    assert(tree->root->values[2] == &v30);

    bptree_destroy(tree);
}

static void test_insert_updates_duplicate_key_value(void)
{
    BPTree *tree = bptree_create();
    static int first_value = 1;
    static int second_value = 2;

    assert(tree != NULL);
    assert(bptree_insert(tree, 15, &first_value) == 0);
    assert(bptree_insert(tree, 15, &second_value) == 0);

    assert(tree->root->key_count == 1);
    assert(tree->root->keys[0] == 15);
    assert(bptree_search(tree, 15) == &second_value);

    bptree_destroy(tree);
}

static void test_insert_splits_root_leaf_when_full(void)
{
    BPTree *tree = bptree_create();
    static int v10 = 10;
    static int v20 = 20;
    static int v30 = 30;
    static int v40 = 40;

    assert(tree != NULL);
    assert(bptree_insert(tree, 10, &v10) == 0);
    assert(bptree_insert(tree, 20, &v20) == 0);
    assert(bptree_insert(tree, 30, &v30) == 0);

    assert(bptree_insert(tree, 40, &v40) == 0);

    assert(tree->root->is_leaf == 0);
    assert(tree->root->key_count == 1);
    assert(tree->root->keys[0] == 30);
    assert(tree->root->children[0] != NULL);
    assert(tree->root->children[1] != NULL);
    assert(tree->first_leaf == tree->root->children[0]);

    assert(tree->root->children[0]->is_leaf == 1);
    assert(tree->root->children[0]->key_count == 2);
    assert(tree->root->children[0]->keys[0] == 10);
    assert(tree->root->children[0]->keys[1] == 20);

    assert(tree->root->children[1]->is_leaf == 1);
    assert(tree->root->children[1]->key_count == 2);
    assert(tree->root->children[1]->keys[0] == 30);
    assert(tree->root->children[1]->keys[1] == 40);
    assert(tree->root->children[0]->next == tree->root->children[1]);

    assert(bptree_search(tree, 10) == &v10);
    assert(bptree_search(tree, 20) == &v20);
    assert(bptree_search(tree, 30) == &v30);
    assert(bptree_search(tree, 40) == &v40);

    bptree_destroy(tree);
}

static void test_insert_splits_leaf_and_inserts_into_parent(void)
{
    BPTree *tree = bptree_create();
    static int v10 = 10;
    static int v20 = 20;
    static int v30 = 30;
    static int v35 = 35;
    static int v40 = 40;
    static int v50 = 50;

    assert(tree != NULL);

    assert(bptree_insert(tree, 10, &v10) == 0);
    assert(bptree_insert(tree, 20, &v20) == 0);
    assert(bptree_insert(tree, 30, &v30) == 0);
    assert(bptree_insert(tree, 40, &v40) == 0);

    /* 여기까지 오면 root = [30], leaves = [10,20] [30,40] */
    assert(bptree_insert(tree, 35, &v35) == 0);
    assert(bptree_insert(tree, 50, &v50) == 0);

    assert(tree->root->is_leaf == 0);
    assert(tree->root->key_count == 2);
    assert(tree->root->keys[0] == 30);
    assert(tree->root->keys[1] == 40);

    assert(tree->root->children[0] != NULL);
    assert(tree->root->children[1] != NULL);
    assert(tree->root->children[2] != NULL);

    assert(tree->root->children[0]->key_count == 2);
    assert(tree->root->children[0]->keys[0] == 10);
    assert(tree->root->children[0]->keys[1] == 20);

    assert(tree->root->children[1]->key_count == 2);
    assert(tree->root->children[1]->keys[0] == 30);
    assert(tree->root->children[1]->keys[1] == 35);

    assert(tree->root->children[2]->key_count == 2);
    assert(tree->root->children[2]->keys[0] == 40);
    assert(tree->root->children[2]->keys[1] == 50);

    assert(tree->first_leaf != NULL);
    assert(tree->first_leaf == tree->root->children[0]);
    assert(tree->root->children[0]->next == tree->root->children[1]);
    assert(tree->root->children[1]->next == tree->root->children[2]);

    assert(bptree_search(tree, 10) == &v10);
    assert(bptree_search(tree, 20) == &v20);
    assert(bptree_search(tree, 30) == &v30);
    assert(bptree_search(tree, 35) == &v35);
    assert(bptree_search(tree, 40) == &v40);
    assert(bptree_search(tree, 50) == &v50);

    bptree_destroy(tree);
}

static void run_test(const TestCase *test_case, int index, int total)
{
    printf("[INSERT %d/%d] %s ... ", index, total, test_case->name);
    fflush(stdout);
    test_case->fn();
    printf("PASS\n");
}

int main(void)
{
    TestCase tests[] = {
        {"case-01: 빈 리프 노드에 첫 key 를 넣는다", test_insert_into_empty_leaf},
        {"case-02: 여러 key 를 넣어도 정렬 순서를 유지한다", test_insert_keeps_sorted_order},
        {"case-03: 같은 key 를 다시 넣으면 값을 갱신한다", test_insert_updates_duplicate_key_value},
        {"case-04: 루트 리프가 가득 차면 두 리프로 split 된다", test_insert_splits_root_leaf_when_full},
        {"case-05: 부모가 여유 있으면 리프 split 결과를 부모에 반영한다", test_insert_splits_leaf_and_inserts_into_parent},
    };
    int total = (int)(sizeof(tests) / sizeof(tests[0]));
    int i;

    for (i = 0; i < total; i++) {
        run_test(&tests[i], i + 1, total);
    }

    printf("Insert tests passed! (%d/%d)\n", total, total);
    return 0;
}
