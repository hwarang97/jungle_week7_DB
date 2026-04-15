#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define DEFAULT_QUERY_COUNT 5000
#define DEFAULT_ORDER 4
#define MIN_ORDER 4
#define MAX_ORDER 1024

typedef struct {
    int id;
    int payload;
    char name[32];
} Record;

typedef struct BTreeNode {
    int leaf;
    int key_count;
    int *keys;
    int *values;
    struct BTreeNode **children;
} BTreeNode;

typedef struct {
    BTreeNode *root;
    int order;
    int max_keys;
    int min_degree;
    long long node_count;
    long long split_count;
} BTree;

typedef struct BPlusNode {
    int leaf;
    int key_count;
    int *keys;
    int *values;
    struct BPlusNode **children;
    struct BPlusNode *next;
} BPlusNode;

typedef struct {
    int promoted_key;
    BPlusNode *right;
} BPlusSplit;

typedef struct {
    BPlusNode *root;
    int order;
    int max_keys;
    long long node_count;
    long long split_count;
} BPlusTree;

typedef struct {
    double total_ms;
    double avg_ns;
    long long comparisons;
    long long found;
    long long checksum;
} SearchMetric;

typedef struct {
    const char *name;
    double build_ms;
    long long insert_comparisons;
    SearchMetric search;
    int height;
    long long node_count;
    long long split_count;
} IndexMetric;

typedef struct {
    int size;
    int query_count;
    int order;
    int max_keys;
    double table_build_ms;
    long long table_insert_comparisons;
    SearchMetric full_scan;
    IndexMetric btree;
    IndexMetric bplus;
} BenchmarkResult;

static double now_ms(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
#endif
}

static int normalize_order(int order)
{
    if (order < MIN_ORDER) {
        order = MIN_ORDER;
    }
    if (order > MAX_ORDER) {
        order = MAX_ORDER;
    }
    if ((order % 2) != 0) {
        ++order;
    }
    if (order > MAX_ORDER) {
        order = MAX_ORDER;
    }
    return order;
}

static unsigned int next_rand(unsigned int *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static Record *build_table(int size)
{
    Record *records = (Record *)malloc(sizeof(Record) * (size_t)size);
    if (!records) {
        return NULL;
    }

    for (int i = 0; i < size; ++i) {
        records[i].id = i + 1;
        records[i].payload = (i + 1) * 3;
        snprintf(records[i].name, sizeof(records[i].name), "user%d", i + 1);
    }

    return records;
}

static int *build_queries(int size, int query_count)
{
    int *queries = (int *)malloc(sizeof(int) * (size_t)query_count);
    unsigned int state = 0xC0FFEEu;

    if (!queries) {
        return NULL;
    }

    for (int i = 0; i < query_count; ++i) {
        queries[i] = (int)(next_rand(&state) % (unsigned int)size) + 1;
    }

    return queries;
}

static int full_scan_find(const Record *records, int size, int key, long long *comparisons)
{
    for (int i = 0; i < size; ++i) {
        ++(*comparisons);
        if (records[i].id == key) {
            return i;
        }
    }
    return -1;
}

static SearchMetric run_full_scan(const Record *records, int size, const int *queries, int query_count)
{
    SearchMetric metric = {0};
    double start = now_ms();

    for (int i = 0; i < query_count; ++i) {
        int index = full_scan_find(records, size, queries[i], &metric.comparisons);
        if (index >= 0) {
            ++metric.found;
            metric.checksum += records[index].id;
        }
    }

    metric.total_ms = now_ms() - start;
    metric.avg_ns = metric.total_ms * 1000000.0 / (double)query_count;
    return metric;
}

static BTreeNode *btree_create_node(BTree *tree, int leaf)
{
    BTreeNode *node = (BTreeNode *)calloc(1, sizeof(BTreeNode));
    if (!node) {
        return NULL;
    }

    node->leaf = leaf;
    node->keys = (int *)calloc((size_t)tree->max_keys, sizeof(int));
    node->values = (int *)calloc((size_t)tree->max_keys, sizeof(int));
    node->children = (BTreeNode **)calloc((size_t)tree->order, sizeof(BTreeNode *));

    if (!node->keys || !node->values || !node->children) {
        free(node->keys);
        free(node->values);
        free(node->children);
        free(node);
        return NULL;
    }

    ++tree->node_count;
    return node;
}

static BTree *btree_create(int order)
{
    BTree *tree = (BTree *)calloc(1, sizeof(BTree));
    if (!tree) {
        return NULL;
    }

    tree->order = normalize_order(order);
    tree->max_keys = tree->order - 1;
    tree->min_degree = tree->order / 2;
    tree->root = btree_create_node(tree, 1);
    if (!tree->root) {
        free(tree);
        return NULL;
    }
    return tree;
}

static int btree_split_child(BTree *tree, BTreeNode *parent, int child_index)
{
    BTreeNode *full = parent->children[child_index];
    BTreeNode *right = btree_create_node(tree, full->leaf);
    int t = tree->min_degree;

    if (!right) {
        return -1;
    }

    right->key_count = t - 1;
    for (int j = 0; j < t - 1; ++j) {
        right->keys[j] = full->keys[j + t];
        right->values[j] = full->values[j + t];
    }

    if (!full->leaf) {
        for (int j = 0; j < t; ++j) {
            right->children[j] = full->children[j + t];
            full->children[j + t] = NULL;
        }
    }

    full->key_count = t - 1;

    for (int j = parent->key_count; j >= child_index + 1; --j) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[child_index + 1] = right;

    for (int j = parent->key_count - 1; j >= child_index; --j) {
        parent->keys[j + 1] = parent->keys[j];
        parent->values[j + 1] = parent->values[j];
    }
    parent->keys[child_index] = full->keys[t - 1];
    parent->values[child_index] = full->values[t - 1];
    ++parent->key_count;
    ++tree->split_count;
    return 0;
}

static int btree_insert_nonfull(BTree *tree, BTreeNode *node, int key, int value, long long *comparisons)
{
    int i = node->key_count - 1;

    if (node->leaf) {
        while (i >= 0) {
            ++(*comparisons);
            if (key >= node->keys[i]) {
                break;
            }
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            --i;
        }
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        ++node->key_count;
        return 0;
    }

    while (i >= 0) {
        ++(*comparisons);
        if (key >= node->keys[i]) {
            break;
        }
        --i;
    }
    ++i;

    if (node->children[i]->key_count == tree->max_keys) {
        if (btree_split_child(tree, node, i) != 0) {
            return -1;
        }
        ++(*comparisons);
        if (key > node->keys[i]) {
            ++i;
        }
    }

    return btree_insert_nonfull(tree, node->children[i], key, value, comparisons);
}

static int btree_insert(BTree *tree, int key, int value, long long *comparisons)
{
    BTreeNode *root = tree->root;

    if (root->key_count == tree->max_keys) {
        BTreeNode *new_root = btree_create_node(tree, 0);
        if (!new_root) {
            return -1;
        }
        new_root->children[0] = root;
        tree->root = new_root;
        if (btree_split_child(tree, new_root, 0) != 0) {
            return -1;
        }
        return btree_insert_nonfull(tree, new_root, key, value, comparisons);
    }

    return btree_insert_nonfull(tree, root, key, value, comparisons);
}

static int btree_search_node(const BTreeNode *node, int key, long long *comparisons)
{
    int i = 0;

    while (i < node->key_count) {
        ++(*comparisons);
        if (key <= node->keys[i]) {
            break;
        }
        ++i;
    }

    if (i < node->key_count) {
        ++(*comparisons);
        if (key == node->keys[i]) {
            return node->values[i];
        }
    }

    if (node->leaf) {
        return -1;
    }

    return btree_search_node(node->children[i], key, comparisons);
}

static int btree_search(const BTree *tree, int key, long long *comparisons)
{
    return btree_search_node(tree->root, key, comparisons);
}

static int btree_height_node(const BTreeNode *node)
{
    if (!node) {
        return 0;
    }
    if (node->leaf) {
        return 1;
    }
    return 1 + btree_height_node(node->children[0]);
}

static void btree_free_node(BTreeNode *node)
{
    if (!node) {
        return;
    }
    if (!node->leaf) {
        for (int i = 0; i <= node->key_count; ++i) {
            btree_free_node(node->children[i]);
        }
    }
    free(node->keys);
    free(node->values);
    free(node->children);
    free(node);
}

static void btree_free(BTree *tree)
{
    if (!tree) {
        return;
    }
    btree_free_node(tree->root);
    free(tree);
}

static BPlusNode *bplus_create_node(BPlusTree *tree, int leaf)
{
    BPlusNode *node = (BPlusNode *)calloc(1, sizeof(BPlusNode));
    if (!node) {
        return NULL;
    }

    node->leaf = leaf;
    node->keys = (int *)calloc((size_t)tree->max_keys + 1, sizeof(int));
    node->values = (int *)calloc((size_t)tree->max_keys + 1, sizeof(int));
    node->children = (BPlusNode **)calloc((size_t)tree->order + 1, sizeof(BPlusNode *));

    if (!node->keys || !node->values || !node->children) {
        free(node->keys);
        free(node->values);
        free(node->children);
        free(node);
        return NULL;
    }

    ++tree->node_count;
    return node;
}

static BPlusTree *bplus_create(int order)
{
    BPlusTree *tree = (BPlusTree *)calloc(1, sizeof(BPlusTree));
    if (!tree) {
        return NULL;
    }

    tree->order = normalize_order(order);
    tree->max_keys = tree->order - 1;
    tree->root = bplus_create_node(tree, 1);
    if (!tree->root) {
        free(tree);
        return NULL;
    }
    return tree;
}

static int bplus_child_index(const BPlusNode *node, int key, long long *comparisons)
{
    int i = 0;
    while (i < node->key_count) {
        ++(*comparisons);
        if (key < node->keys[i]) {
            break;
        }
        ++i;
    }
    return i;
}

static int bplus_insert_recursive(BPlusTree *tree, BPlusNode *node, int key, int value,
                                  BPlusSplit *split, long long *comparisons)
{
    int i;

    split->right = NULL;
    split->promoted_key = 0;

    if (node->leaf) {
        i = node->key_count - 1;
        while (i >= 0) {
            ++(*comparisons);
            if (key >= node->keys[i]) {
                break;
            }
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            --i;
        }
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        ++node->key_count;

        if (node->key_count <= tree->max_keys) {
            return 0;
        }

        int split_at = node->key_count / 2;
        BPlusNode *right = bplus_create_node(tree, 1);
        if (!right) {
            return -1;
        }

        right->key_count = node->key_count - split_at;
        for (int j = 0; j < right->key_count; ++j) {
            right->keys[j] = node->keys[split_at + j];
            right->values[j] = node->values[split_at + j];
        }
        node->key_count = split_at;
        right->next = node->next;
        node->next = right;

        split->promoted_key = right->keys[0];
        split->right = right;
        ++tree->split_count;
        return 0;
    }

    i = bplus_child_index(node, key, comparisons);
    BPlusSplit child_split;
    if (bplus_insert_recursive(tree, node->children[i], key, value, &child_split, comparisons) != 0) {
        return -1;
    }

    if (!child_split.right) {
        return 0;
    }

    for (int j = node->key_count; j > i; --j) {
        node->keys[j] = node->keys[j - 1];
    }
    for (int j = node->key_count + 1; j > i + 1; --j) {
        node->children[j] = node->children[j - 1];
    }
    node->keys[i] = child_split.promoted_key;
    node->children[i + 1] = child_split.right;
    ++node->key_count;

    if (node->key_count <= tree->max_keys) {
        return 0;
    }

    int total_keys = node->key_count;
    int middle = total_keys / 2;
    BPlusNode *right = bplus_create_node(tree, 0);
    if (!right) {
        return -1;
    }

    split->promoted_key = node->keys[middle];
    right->key_count = total_keys - middle - 1;

    for (int j = 0; j < right->key_count; ++j) {
        right->keys[j] = node->keys[middle + 1 + j];
    }
    for (int j = 0; j <= right->key_count; ++j) {
        right->children[j] = node->children[middle + 1 + j];
    }

    node->key_count = middle;
    split->right = right;
    ++tree->split_count;
    return 0;
}

static int bplus_insert(BPlusTree *tree, int key, int value, long long *comparisons)
{
    BPlusSplit split;
    if (bplus_insert_recursive(tree, tree->root, key, value, &split, comparisons) != 0) {
        return -1;
    }

    if (split.right) {
        BPlusNode *new_root = bplus_create_node(tree, 0);
        if (!new_root) {
            return -1;
        }
        new_root->keys[0] = split.promoted_key;
        new_root->children[0] = tree->root;
        new_root->children[1] = split.right;
        new_root->key_count = 1;
        tree->root = new_root;
    }

    return 0;
}

static int bplus_search(const BPlusTree *tree, int key, long long *comparisons)
{
    BPlusNode *node = tree->root;

    while (node && !node->leaf) {
        int i = 0;
        while (i < node->key_count) {
            ++(*comparisons);
            if (key < node->keys[i]) {
                break;
            }
            ++i;
        }
        node = node->children[i];
    }

    if (!node) {
        return -1;
    }

    for (int i = 0; i < node->key_count; ++i) {
        ++(*comparisons);
        if (key == node->keys[i]) {
            return node->values[i];
        }
        if (key < node->keys[i]) {
            return -1;
        }
    }

    return -1;
}

static int bplus_height_node(const BPlusNode *node)
{
    if (!node) {
        return 0;
    }
    if (node->leaf) {
        return 1;
    }
    return 1 + bplus_height_node(node->children[0]);
}

static void bplus_free_node(BPlusNode *node)
{
    if (!node) {
        return;
    }
    if (!node->leaf) {
        for (int i = 0; i <= node->key_count; ++i) {
            bplus_free_node(node->children[i]);
        }
    }
    free(node->keys);
    free(node->values);
    free(node->children);
    free(node);
}

static void bplus_free(BPlusTree *tree)
{
    if (!tree) {
        return;
    }
    bplus_free_node(tree->root);
    free(tree);
}

static SearchMetric run_btree_search(const BTree *tree, const Record *records, const int *queries, int query_count)
{
    SearchMetric metric = {0};
    double start = now_ms();

    for (int i = 0; i < query_count; ++i) {
        int index = btree_search(tree, queries[i], &metric.comparisons);
        if (index >= 0) {
            ++metric.found;
            metric.checksum += records[index].id;
        }
    }

    metric.total_ms = now_ms() - start;
    metric.avg_ns = metric.total_ms * 1000000.0 / (double)query_count;
    return metric;
}

static SearchMetric run_bplus_search(const BPlusTree *tree, const Record *records, const int *queries, int query_count)
{
    SearchMetric metric = {0};
    double start = now_ms();

    for (int i = 0; i < query_count; ++i) {
        int index = bplus_search(tree, queries[i], &metric.comparisons);
        if (index >= 0) {
            ++metric.found;
            metric.checksum += records[index].id;
        }
    }

    metric.total_ms = now_ms() - start;
    metric.avg_ns = metric.total_ms * 1000000.0 / (double)query_count;
    return metric;
}

static int run_one(int size, int query_count, int order, BenchmarkResult *out)
{
    Record *records = NULL;
    int *queries = NULL;
    BTree *btree = NULL;
    BPlusTree *bplus = NULL;
    double start;

    memset(out, 0, sizeof(*out));
    out->size = size;
    out->query_count = query_count;
    out->order = normalize_order(order);
    out->max_keys = out->order - 1;

    start = now_ms();
    records = build_table(size);
    out->table_build_ms = now_ms() - start;
    if (!records) {
        goto fail;
    }

    queries = build_queries(size, query_count);
    if (!queries) {
        goto fail;
    }

    btree = btree_create(out->order);
    if (!btree) {
        goto fail;
    }
    start = now_ms();
    for (int i = 0; i < size; ++i) {
        if (btree_insert(btree, records[i].id, i, &out->btree.insert_comparisons) != 0) {
            goto fail;
        }
    }
    out->btree.name = "B-Tree";
    out->btree.build_ms = now_ms() - start;
    out->btree.height = btree_height_node(btree->root);
    out->btree.node_count = btree->node_count;
    out->btree.split_count = btree->split_count;

    bplus = bplus_create(out->order);
    if (!bplus) {
        goto fail;
    }
    start = now_ms();
    for (int i = 0; i < size; ++i) {
        if (bplus_insert(bplus, records[i].id, i, &out->bplus.insert_comparisons) != 0) {
            goto fail;
        }
    }
    out->bplus.name = "B+Tree";
    out->bplus.build_ms = now_ms() - start;
    out->bplus.height = bplus_height_node(bplus->root);
    out->bplus.node_count = bplus->node_count;
    out->bplus.split_count = bplus->split_count;

    out->full_scan = run_full_scan(records, size, queries, query_count);
    out->btree.search = run_btree_search(btree, records, queries, query_count);
    out->bplus.search = run_bplus_search(bplus, records, queries, query_count);

    free(queries);
    btree_free(btree);
    bplus_free(bplus);
    free(records);
    return 0;

fail:
    free(queries);
    btree_free(btree);
    bplus_free(bplus);
    free(records);
    return -1;
}

static void print_search_json(const SearchMetric *metric)
{
    printf("{\"total_ms\":%.6f,\"avg_ns\":%.3f,\"comparisons\":%lld,\"found\":%lld,\"checksum\":%lld}",
           metric->total_ms,
           metric->avg_ns,
           metric->comparisons,
           metric->found,
           metric->checksum);
}

static void print_index_json(const IndexMetric *metric)
{
    printf("{\"name\":\"%s\",\"build_ms\":%.6f,\"insert_comparisons\":%lld,\"height\":%d,\"nodes\":%lld,\"splits\":%lld,\"search\":",
           metric->name,
           metric->build_ms,
           metric->insert_comparisons,
           metric->height,
           metric->node_count,
           metric->split_count);
    print_search_json(&metric->search);
    printf("}");
}

static void print_result_json(const BenchmarkResult *result)
{
    double btree_speedup = result->btree.search.total_ms > 0.0
        ? result->full_scan.total_ms / result->btree.search.total_ms
        : 0.0;
    double bplus_speedup = result->bplus.search.total_ms > 0.0
        ? result->full_scan.total_ms / result->bplus.search.total_ms
        : 0.0;
    double btree_insert_total_ms = result->table_build_ms + result->btree.build_ms;
    double bplus_insert_total_ms = result->table_build_ms + result->bplus.build_ms;
    double btree_insert_speedup = btree_insert_total_ms > 0.0
        ? result->table_build_ms / btree_insert_total_ms
        : 0.0;
    double bplus_insert_speedup = bplus_insert_total_ms > 0.0
        ? result->table_build_ms / bplus_insert_total_ms
        : 0.0;

    printf("{");
    printf("\"size\":%d,", result->size);
    printf("\"query_count\":%d,", result->query_count);
    printf("\"order\":%d,", result->order);
    printf("\"max_keys\":%d,", result->max_keys);
    printf("\"table_build_ms\":%.6f,", result->table_build_ms);
    printf("\"table_insert_comparisons\":%lld,", result->table_insert_comparisons);
    printf("\"full_scan\":");
    print_search_json(&result->full_scan);
    printf(",\"btree\":");
    print_index_json(&result->btree);
    printf(",\"bplus_tree\":");
    print_index_json(&result->bplus);
    printf(",\"speedup\":{\"btree_vs_full_scan\":%.3f,\"bplus_vs_full_scan\":%.3f,"
           "\"btree_insert_vs_table\":%.3f,\"bplus_insert_vs_table\":%.3f}",
           btree_speedup,
           bplus_speedup,
           btree_insert_speedup,
           bplus_insert_speedup);
    printf("}");
}

static void print_result_pretty(const BenchmarkResult *result)
{
    printf("rows=%d queries=%d order=%d max_keys=%d\n",
           result->size, result->query_count, result->order, result->max_keys);
    printf("table build: %.3f ms\n", result->table_build_ms);
    printf("full scan : %.3f ms total, %.1f ns avg, comparisons=%lld\n",
           result->full_scan.total_ms,
           result->full_scan.avg_ns,
           result->full_scan.comparisons);
    printf("B-Tree    : build %.3f ms, search %.3f ms, %.1f ns avg, height=%d, nodes=%lld, splits=%lld\n",
           result->btree.build_ms,
           result->btree.search.total_ms,
           result->btree.search.avg_ns,
           result->btree.height,
           result->btree.node_count,
           result->btree.split_count);
    printf("            insert comparisons=%lld, search comparisons=%lld\n",
           result->btree.insert_comparisons,
           result->btree.search.comparisons);
    printf("B+Tree   : build %.3f ms, search %.3f ms, %.1f ns avg, height=%d, nodes=%lld, splits=%lld\n",
           result->bplus.build_ms,
           result->bplus.search.total_ms,
           result->bplus.search.avg_ns,
           result->bplus.height,
           result->bplus.node_count,
           result->bplus.split_count);
    printf("            insert comparisons=%lld, search comparisons=%lld\n",
           result->bplus.insert_comparisons,
           result->bplus.search.comparisons);
}

static Record *build_permuted_table(int size)
{
    Record *records = (Record *)malloc(sizeof(Record) * (size_t)size);
    if (!records) {
        return NULL;
    }

    for (int i = 0; i < size; ++i) {
        int id = (int)(((long long)i * 37) % size) + 1;
        records[i].id = id;
        records[i].payload = i * 11;
        snprintf(records[i].name, sizeof(records[i].name), "row%d_at_%d", id, i);
    }

    return records;
}

static int verify_table_records(const Record *records, int size)
{
    int *seen = (int *)calloc((size_t)size + 1, sizeof(int));
    if (!seen) {
        printf("[FAIL] table insert: allocation failed\n");
        return -1;
    }

    for (int i = 0; i < size; ++i) {
        if (records[i].id < 1 || records[i].id > size) {
            printf("[FAIL] table insert: id out of range at row %d\n", i);
            free(seen);
            return -1;
        }
        if (seen[records[i].id]) {
            printf("[FAIL] table insert: duplicate id %d\n", records[i].id);
            free(seen);
            return -1;
        }
        seen[records[i].id] = 1;
    }

    for (int id = 1; id <= size; ++id) {
        if (!seen[id]) {
            printf("[FAIL] table insert: missing id %d\n", id);
            free(seen);
            return -1;
        }
    }

    free(seen);
    printf("[PASS] table insert: %d real records stored with unique ids\n", size);
    return 0;
}

static int verify_index_update_and_search(int size, int order)
{
    Record *records = build_permuted_table(size);
    BTree *btree = NULL;
    BPlusTree *bplus = NULL;
    int status = -1;

    if (!records) {
        printf("[FAIL] verify: record allocation failed\n");
        return -1;
    }
    if (verify_table_records(records, size) != 0) {
        goto cleanup;
    }

    btree = btree_create(order);
    bplus = bplus_create(order);
    if (!btree || !bplus) {
        printf("[FAIL] index build: tree allocation failed\n");
        goto cleanup;
    }

    for (int i = 0; i < size; ++i) {
        long long btree_insert_comparisons = 0;
        long long bplus_insert_comparisons = 0;
        if (btree_insert(btree, records[i].id, i, &btree_insert_comparisons) != 0 ||
            bplus_insert(bplus, records[i].id, i, &bplus_insert_comparisons) != 0) {
            printf("[FAIL] index build: insert failed at row %d\n", i);
            goto cleanup;
        }
    }

    if (btree->node_count <= 1 || bplus->node_count <= 1 ||
        btree->split_count <= 0 || bplus->split_count <= 0) {
        printf("[FAIL] index update: expected nodes and splits after many inserts\n");
        goto cleanup;
    }

    printf("[PASS] index update: B-Tree nodes=%lld splits=%lld, B+Tree nodes=%lld splits=%lld\n",
           btree->node_count, btree->split_count,
           bplus->node_count, bplus->split_count);

    for (int key = 1; key <= size; ++key) {
        long long full_comparisons = 0;
        long long btree_comparisons = 0;
        long long bplus_comparisons = 0;
        int expected = full_scan_find(records, size, key, &full_comparisons);
        int btree_index = btree_search(btree, key, &btree_comparisons);
        int bplus_index = bplus_search(bplus, key, &bplus_comparisons);

        if (expected < 0 || btree_index != expected || bplus_index != expected) {
            printf("[FAIL] index search: key %d expected row %d, B-Tree %d, B+Tree %d\n",
                   key, expected, btree_index, bplus_index);
            goto cleanup;
        }
        if (records[btree_index].id != key || records[bplus_index].id != key) {
            printf("[FAIL] index search: returned row does not contain key %d\n", key);
            goto cleanup;
        }
    }

    printf("[PASS] index search: B-Tree/B+Tree return the same record positions as full scan\n");

    {
        long long full_comparisons = 0;
        long long btree_comparisons = 0;
        long long bplus_comparisons = 0;
        int missing = size + 123;
        int full_index = full_scan_find(records, size, missing, &full_comparisons);
        int btree_index = btree_search(btree, missing, &btree_comparisons);
        int bplus_index = bplus_search(bplus, missing, &bplus_comparisons);

        if (full_index != -1 || btree_index != -1 || bplus_index != -1) {
            printf("[FAIL] missing key: expected all methods to return not found\n");
            goto cleanup;
        }
        if (full_comparisons != size ||
            btree_comparisons >= full_comparisons ||
            bplus_comparisons >= full_comparisons) {
            printf("[FAIL] missing key: expected index comparisons to be below full scan (%lld, %lld, %lld)\n",
                   full_comparisons, btree_comparisons, bplus_comparisons);
            goto cleanup;
        }
        printf("[PASS] missing key: indexes return not found without scanning every record\n");
    }

    status = 0;

cleanup:
    btree_free(btree);
    bplus_free(bplus);
    free(records);
    return status;
}

static int verify_order_changes_structure(void)
{
    BenchmarkResult small_order;
    BenchmarkResult large_order;
    const int size = 4096;
    const int queries = 256;

    if (run_one(size, queries, 4, &small_order) != 0 ||
        run_one(size, queries, 100, &large_order) != 0) {
        printf("[FAIL] order verify: benchmark run failed\n");
        return -1;
    }

    if (small_order.order != 4 || small_order.max_keys != 3 ||
        large_order.order != 100 || large_order.max_keys != 99) {
        printf("[FAIL] order verify: order/max_keys not reflected in JSON model\n");
        return -1;
    }

    if (large_order.btree.height >= small_order.btree.height ||
        large_order.bplus.height >= small_order.bplus.height ||
        large_order.btree.split_count >= small_order.btree.split_count ||
        large_order.bplus.split_count >= small_order.bplus.split_count) {
        printf("[FAIL] order verify: larger order did not reduce height/splits as expected\n");
        printf("       order4  BTree h=%d splits=%lld, B+ h=%d splits=%lld\n",
               small_order.btree.height, small_order.btree.split_count,
               small_order.bplus.height, small_order.bplus.split_count);
        printf("       order100 BTree h=%d splits=%lld, B+ h=%d splits=%lld\n",
               large_order.btree.height, large_order.btree.split_count,
               large_order.bplus.height, large_order.bplus.split_count);
        return -1;
    }

    printf("[PASS] order: order=4 produced max_keys=3, order=100 produced max_keys=99\n");
    printf("[PASS] order: larger order changed tree shape (B-Tree h %d -> %d, B+Tree h %d -> %d)\n",
           small_order.btree.height, large_order.btree.height,
           small_order.bplus.height, large_order.bplus.height);
    return 0;
}

static int run_verify(void)
{
    int status = 0;

    printf("[VERIFY] db_benchmark integrity checks\n");
    if (verify_index_update_and_search(4096, 4) != 0) {
        status = 1;
    }
    if (verify_order_changes_structure() != 0) {
        status = 1;
    }

    if (status == 0) {
        printf("[VERIFY] all checks passed\n");
    } else {
        printf("[VERIFY] checks failed\n");
    }
    return status;
}

static int parse_int_arg(const char *text, int fallback)
{
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (!text || !*text || (end && *end) || value <= 0 || value > 100000000) {
        return fallback;
    }
    return (int)value;
}

int main(int argc, char **argv)
{
    int size = 100000;
    int query_count = DEFAULT_QUERY_COUNT;
    int order = DEFAULT_ORDER;
    int json = 0;
    int all = 0;
    int verify = 0;
    const int sizes[] = {10000, 100000, 1000000};

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            size = parse_int_arg(argv[++i], size);
        } else if (strcmp(argv[i], "--queries") == 0 && i + 1 < argc) {
            query_count = parse_int_arg(argv[++i], query_count);
        } else if (strcmp(argv[i], "--order") == 0 && i + 1 < argc) {
            order = normalize_order(parse_int_arg(argv[++i], order));
        } else if (strcmp(argv[i], "--json") == 0) {
            json = 1;
        } else if (strcmp(argv[i], "--all") == 0) {
            all = 1;
        } else if (strcmp(argv[i], "--verify") == 0) {
            verify = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [--size N | --all] [--queries N] [--order N] [--json] [--verify]\n", argv[0]);
            printf("  --order is normalized to an even value from %d to %d.\n", MIN_ORDER, MAX_ORDER);
            return 0;
        }
    }

    if (verify) {
        return run_verify();
    }

    order = normalize_order(order);

    if (all) {
        if (json) {
            printf("{\"order\":%d,\"max_keys\":%d,\"results\":[", order, order - 1);
        }
        for (int i = 0; i < 3; ++i) {
            BenchmarkResult result;
            if (run_one(sizes[i], query_count, order, &result) != 0) {
                fprintf(stderr, "benchmark failed for size %d order %d\n", sizes[i], order);
                return 1;
            }
            if (json) {
                if (i > 0) {
                    printf(",");
                }
                print_result_json(&result);
            } else {
                if (i > 0) {
                    printf("\n");
                }
                print_result_pretty(&result);
            }
        }
        if (json) {
            printf("]}\n");
        }
        return 0;
    }

    BenchmarkResult result;
    if (run_one(size, query_count, order, &result) != 0) {
        fprintf(stderr, "benchmark failed\n");
        return 1;
    }

    if (json) {
        print_result_json(&result);
        printf("\n");
    } else {
        print_result_pretty(&result);
    }

    return 0;
}
