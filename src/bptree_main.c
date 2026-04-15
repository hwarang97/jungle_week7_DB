#include "bptree.h"

#include <locale.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

static void configure_console_utf8(void)
{
    setlocale(LC_ALL, ".UTF-8");

#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

static void print_root_state(const BPTree *tree)
{
    int index;

    if (!tree || !tree->root) {
        return;
    }

    printf("root (%s):", tree->root->is_leaf ? "leaf" : "internal");
    for (index = 0; index < tree->root->key_count; index++) {
        printf(" %d", tree->root->keys[index]);
    }
    printf("\n");
}

static void print_leaf_chain(const BPTree *tree)
{
    BPTreeNode *leaf;
    int index;

    if (!tree) {
        return;
    }

    printf("leaf chain:");
    for (leaf = tree->first_leaf; leaf != NULL; leaf = leaf->next) {
        printf(" [");
        for (index = 0; index < leaf->key_count; index++) {
            if (index > 0) {
                printf(", ");
            }
            printf("%d", leaf->keys[index]);
        }
        printf("]");
    }
    printf("\n");
}

static void print_search_result(BPTree *tree, int key)
{
    int *result = (int *)bptree_search(tree, key);

    if (result) {
        printf("search(%d) -> found value %d\n", key, *result);
        return;
    }

    printf("search(%d) -> not found\n", key);
}

int main(void)
{
    BPTree *tree = bptree_create();
    int values[6] = {10, 20, 30, 40, 50, 60};
    int keys[6] = {10, 20, 30, 40, 50, 60};
    int index;

    configure_console_utf8();

    if (!tree) {
        return 1;
    }

    printf("B+ Tree Index Project\n");
    for (index = 0; index < 6; index++) {
        bptree_insert(tree, keys[index], &values[index]);
    }

    print_root_state(tree);
    print_leaf_chain(tree);
    print_search_result(tree, 40);
    print_search_result(tree, 55);

    bptree_destroy(tree);
    return 0;
}
