#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/bptree.h"

#define DEFAULT_ROWS 10000
#define DEFAULT_REQUESTS 5000
#define BTREE_MIN_DEGREE 16
#define BTREE_MAX_KEYS (2 * BTREE_MIN_DEGREE - 1)
#define BTREE_MAX_CHILDREN (2 * BTREE_MIN_DEGREE)

typedef struct {
    int id;
    int age;
    char name[32];
} BenchRow;

typedef struct BTreeNode {
    int is_leaf;
    int key_count;
    int keys[BTREE_MAX_KEYS];
    BenchRow *values[BTREE_MAX_KEYS];
    struct BTreeNode *children[BTREE_MAX_CHILDREN];
} BTreeNode;

typedef struct {
    BTreeNode *root;
} BTree;

typedef struct {
    const char *operation;
    const char *method;
    const char *label;
    double total_ms;
    int matches;
} BenchResult;

static volatile unsigned long long g_sink = 0;

static double elapsed_ms(clock_t start, clock_t end)
{
    return ((double)(end - start) * 1000.0) / (double)CLOCKS_PER_SEC;
}

static BTreeNode *btree_create_node(int is_leaf)
{
    BTreeNode *node = (BTreeNode *)calloc(1, sizeof(*node));
    if (node != NULL) {
        node->is_leaf = is_leaf;
    }
    return node;
}

static BTree *btree_create(void)
{
    BTree *tree = (BTree *)calloc(1, sizeof(*tree));
    if (tree == NULL) {
        return NULL;
    }

    tree->root = btree_create_node(1);
    if (tree->root == NULL) {
        free(tree);
        return NULL;
    }
    return tree;
}

static void btree_destroy_node(BTreeNode *node)
{
    int i;

    if (node == NULL) {
        return;
    }

    if (!node->is_leaf) {
        for (i = 0; i <= node->key_count; ++i) {
            btree_destroy_node(node->children[i]);
        }
    }
    free(node);
}

static void btree_destroy(BTree *tree)
{
    if (tree == NULL) {
        return;
    }
    btree_destroy_node(tree->root);
    free(tree);
}

static BenchRow *btree_search_node(BTreeNode *node, int key)
{
    int i = 0;

    while (i < node->key_count && key > node->keys[i]) {
        ++i;
    }

    if (i < node->key_count && key == node->keys[i]) {
        return node->values[i];
    }

    if (node->is_leaf) {
        return NULL;
    }

    return btree_search_node(node->children[i], key);
}

static BenchRow *btree_search(BTree *tree, int key)
{
    if (tree == NULL || tree->root == NULL) {
        return NULL;
    }
    return btree_search_node(tree->root, key);
}

static int btree_split_child(BTreeNode *parent, int child_index)
{
    BTreeNode *left = parent->children[child_index];
    BTreeNode *right;
    int median_key;
    BenchRow *median_value;
    int i;

    if (left == NULL) {
        return -1;
    }

    right = btree_create_node(left->is_leaf);
    if (right == NULL) {
        return -1;
    }

    right->key_count = BTREE_MIN_DEGREE - 1;
    for (i = 0; i < BTREE_MIN_DEGREE - 1; ++i) {
        right->keys[i] = left->keys[i + BTREE_MIN_DEGREE];
        right->values[i] = left->values[i + BTREE_MIN_DEGREE];
    }

    if (!left->is_leaf) {
        for (i = 0; i < BTREE_MIN_DEGREE; ++i) {
            right->children[i] = left->children[i + BTREE_MIN_DEGREE];
            left->children[i + BTREE_MIN_DEGREE] = NULL;
        }
    }

    median_key = left->keys[BTREE_MIN_DEGREE - 1];
    median_value = left->values[BTREE_MIN_DEGREE - 1];
    left->key_count = BTREE_MIN_DEGREE - 1;

    for (i = parent->key_count; i >= child_index + 1; --i) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[child_index + 1] = right;

    for (i = parent->key_count - 1; i >= child_index; --i) {
        parent->keys[i + 1] = parent->keys[i];
        parent->values[i + 1] = parent->values[i];
    }
    parent->keys[child_index] = median_key;
    parent->values[child_index] = median_value;
    parent->key_count++;
    return 0;
}

static int btree_insert_nonfull(BTreeNode *node, int key, BenchRow *value)
{
    int i = node->key_count - 1;

    if (node->is_leaf) {
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            --i;
        }
        if (i >= 0 && key == node->keys[i]) {
            node->values[i] = value;
            return 0;
        }
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->key_count++;
        return 0;
    }

    while (i >= 0 && key < node->keys[i]) {
        --i;
    }
    if (i >= 0 && key == node->keys[i]) {
        node->values[i] = value;
        return 0;
    }
    ++i;

    if (node->children[i]->key_count == BTREE_MAX_KEYS) {
        if (btree_split_child(node, i) != 0) {
            return -1;
        }
        if (key > node->keys[i]) {
            ++i;
        } else if (key == node->keys[i]) {
            node->values[i] = value;
            return 0;
        }
    }

    return btree_insert_nonfull(node->children[i], key, value);
}

static int btree_insert(BTree *tree, int key, BenchRow *value)
{
    BTreeNode *root;
    BTreeNode *new_root;

    if (tree == NULL || tree->root == NULL) {
        return -1;
    }

    root = tree->root;
    if (root->key_count == BTREE_MAX_KEYS) {
        new_root = btree_create_node(0);
        if (new_root == NULL) {
            return -1;
        }
        new_root->children[0] = root;
        tree->root = new_root;
        if (btree_split_child(new_root, 0) != 0) {
            return -1;
        }
        return btree_insert_nonfull(new_root, key, value);
    }

    return btree_insert_nonfull(root, key, value);
}

static int parse_positive_int(const char *text, int fallback, int max_value)
{
    char *end = NULL;
    long value;

    if (text == NULL || *text == '\0') {
        return fallback;
    }

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value <= 0) {
        return fallback;
    }
    if (value > max_value) {
        return max_value;
    }
    return (int)value;
}

static void prepare_rows(BenchRow *rows, int row_count)
{
    int i;

    for (i = 0; i < row_count; ++i) {
        rows[i].id = i + 1;
        rows[i].age = 20 + (i % 50);
        snprintf(rows[i].name, sizeof(rows[i].name), "user%d", i + 1);
    }
}

static void prepare_query_keys(int *keys, int request_count, int row_count)
{
    uint32_t state = 2166136261u;
    int i;

    for (i = 0; i < request_count; ++i) {
        state = state * 1664525u + 1013904223u;
        keys[i] = (int)(state % (uint32_t)row_count) + 1;
    }
}

static void consume_row(const BenchRow *row)
{
    if (row != NULL) {
        g_sink += (unsigned long long)row->id;
        g_sink += (unsigned long long)row->age;
        g_sink += (unsigned long long)(unsigned char)row->name[0];
    }
}

static BenchResult bench_search_full_scan(const BenchRow *rows, int row_count,
                                          const int *keys, int request_count)
{
    BenchResult result = {"search", "full_scan", "Full scan", 0.0, 0};
    clock_t start = clock();
    int i;

    for (i = 0; i < request_count; ++i) {
        int j;
        int key = keys[i];
        for (j = 0; j < row_count; ++j) {
            if (rows[j].id == key) {
                result.matches++;
                g_sink += (unsigned long long)key;
                break;
            }
        }
    }

    result.total_ms = elapsed_ms(start, clock());
    return result;
}

static BenchResult bench_search_btree(BTree *tree, const int *keys, int request_count)
{
    BenchResult result = {"search", "btree", "B-tree index", 0.0, 0};
    clock_t start = clock();
    int i;

    for (i = 0; i < request_count; ++i) {
        BenchRow *row = btree_search(tree, keys[i]);
        if (row != NULL) {
            result.matches++;
            g_sink += (unsigned long long)row->id;
        }
    }

    result.total_ms = elapsed_ms(start, clock());
    return result;
}

static BenchResult bench_search_bptree(BPTree *tree, const int *keys, int request_count)
{
    BenchResult result = {"search", "bptree", "B+Tree index", 0.0, 0};
    clock_t start = clock();
    int i;

    for (i = 0; i < request_count; ++i) {
        BenchRow *row = (BenchRow *)bptree_search(tree, keys[i]);
        if (row != NULL) {
            result.matches++;
            g_sink += (unsigned long long)row->id;
        }
    }

    result.total_ms = elapsed_ms(start, clock());
    return result;
}

static BenchResult bench_select_full_scan(const BenchRow *rows, int row_count,
                                          const int *keys, int request_count)
{
    BenchResult result = {"select", "full_scan", "Full scan", 0.0, 0};
    clock_t start = clock();
    int i;

    for (i = 0; i < request_count; ++i) {
        int j;
        int key = keys[i];
        for (j = 0; j < row_count; ++j) {
            if (rows[j].id == key) {
                result.matches++;
                consume_row(&rows[j]);
                break;
            }
        }
    }

    result.total_ms = elapsed_ms(start, clock());
    return result;
}

static BenchResult bench_select_btree(BTree *tree, const int *keys, int request_count)
{
    BenchResult result = {"select", "btree", "B-tree index", 0.0, 0};
    clock_t start = clock();
    int i;

    for (i = 0; i < request_count; ++i) {
        BenchRow *row = btree_search(tree, keys[i]);
        if (row != NULL) {
            result.matches++;
            consume_row(row);
        }
    }

    result.total_ms = elapsed_ms(start, clock());
    return result;
}

static BenchResult bench_select_bptree(BPTree *tree, const int *keys, int request_count)
{
    BenchResult result = {"select", "bptree", "B+Tree index", 0.0, 0};
    clock_t start = clock();
    int i;

    for (i = 0; i < request_count; ++i) {
        BenchRow *row = (BenchRow *)bptree_search(tree, keys[i]);
        if (row != NULL) {
            result.matches++;
            consume_row(row);
        }
    }

    result.total_ms = elapsed_ms(start, clock());
    return result;
}

static int build_btree(BTree *tree, BenchRow *rows, int row_count)
{
    int i;

    for (i = 0; i < row_count; ++i) {
        if (btree_insert(tree, rows[i].id, &rows[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static int build_bptree(BPTree *tree, BenchRow *rows, int row_count)
{
    int i;

    for (i = 0; i < row_count; ++i) {
        if (bptree_insert(tree, rows[i].id, &rows[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static void print_json_result(int row_count, int request_count,
                              double btree_build_ms, double bptree_build_ms,
                              const BenchResult *results, int result_count)
{
    int i;

    printf("{\n");
    printf("  \"rows\": %d,\n", row_count);
    printf("  \"requests\": %d,\n", request_count);
    printf("  \"build\": [\n");
    printf("    {\"method\": \"btree\", \"label\": \"B-tree index\", \"total_ms\": %.3f},\n",
           btree_build_ms);
    printf("    {\"method\": \"bptree\", \"label\": \"B+Tree index\", \"total_ms\": %.3f}\n",
           bptree_build_ms);
    printf("  ],\n");
    printf("  \"results\": [\n");
    for (i = 0; i < result_count; ++i) {
        double avg_us = results[i].total_ms * 1000.0 / (double)request_count;
        printf("    {\"operation\": \"%s\", \"method\": \"%s\", \"label\": \"%s\", "
               "\"total_ms\": %.3f, \"avg_us\": %.6f, \"matches\": %d}%s\n",
               results[i].operation,
               results[i].method,
               results[i].label,
               results[i].total_ms,
               avg_us,
               results[i].matches,
               (i + 1 == result_count) ? "" : ",");
    }
    printf("  ],\n");
    printf("  \"sink\": %llu\n", (unsigned long long)g_sink);
    printf("}\n");
}

int main(int argc, char **argv)
{
    int row_count = DEFAULT_ROWS;
    int request_count = DEFAULT_REQUESTS;
    BenchRow *rows = NULL;
    int *keys = NULL;
    BTree *btree = NULL;
    BPTree *bptree = NULL;
    BenchResult results[6];
    clock_t start;
    double btree_build_ms;
    double bptree_build_ms;

    if (argc > 1) {
        row_count = parse_positive_int(argv[1], DEFAULT_ROWS, 1000000);
    }
    if (argc > 2) {
        request_count = parse_positive_int(argv[2], DEFAULT_REQUESTS, 1000000);
    }

    rows = (BenchRow *)malloc((size_t)row_count * sizeof(*rows));
    keys = (int *)malloc((size_t)request_count * sizeof(*keys));
    btree = btree_create();
    bptree = bptree_create();

    if (rows == NULL || keys == NULL || btree == NULL || bptree == NULL) {
        fprintf(stderr, "allocation failed for rows=%d requests=%d\n", row_count, request_count);
        printf("{\"error\":\"allocation failed\",\"rows\":%d,\"requests\":%d}\n",
               row_count, request_count);
        free(rows);
        free(keys);
        btree_destroy(btree);
        bptree_destroy(bptree);
        return 1;
    }

    prepare_rows(rows, row_count);
    prepare_query_keys(keys, request_count, row_count);

    start = clock();
    if (build_btree(btree, rows, row_count) != 0) {
        fprintf(stderr, "B-tree build failed\n");
        return 1;
    }
    btree_build_ms = elapsed_ms(start, clock());

    start = clock();
    if (build_bptree(bptree, rows, row_count) != 0) {
        fprintf(stderr, "B+Tree build failed\n");
        return 1;
    }
    bptree_build_ms = elapsed_ms(start, clock());

    results[0] = bench_search_full_scan(rows, row_count, keys, request_count);
    results[1] = bench_search_btree(btree, keys, request_count);
    results[2] = bench_search_bptree(bptree, keys, request_count);
    results[3] = bench_select_full_scan(rows, row_count, keys, request_count);
    results[4] = bench_select_btree(btree, keys, request_count);
    results[5] = bench_select_bptree(bptree, keys, request_count);

    print_json_result(row_count, request_count, btree_build_ms, bptree_build_ms,
                      results, 6);

    btree_destroy(btree);
    bptree_destroy(bptree);
    free(keys);
    free(rows);
    return 0;
}
