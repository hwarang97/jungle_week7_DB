#include "bptree.h"

#include <stdlib.h>

/* 리프 노드 안에서 새 key 가 들어갈 자리를 찾는다.
 * 이미 같은 key 가 있으면 그 key 의 위치를 돌려준다.
 */
static int bptree_find_insert_position(BPTreeNode *leaf, int key)
{
    int index = 0;

    while (index < leaf->key_count && leaf->keys[index] < key) {
        index++;
    }

    return index;
}

/* 리프 노드 안에 key 와 value 를 정렬 순서를 유지하며 넣는다.
 * 현재 단계에서는 split 을 아직 하지 않으므로,
 * 자리가 없으면 -1 을 돌려준다.
 */
static int bptree_insert_into_leaf(BPTreeNode *leaf, int key, void *value)
{
    int index;
    int move;

    if (!leaf || !leaf->is_leaf) {
        return -1;
    }

    index = bptree_find_insert_position(leaf, key);

    /* 같은 key 가 이미 있으면 값을 새 값으로 바꾼다. */
    if (index < leaf->key_count && leaf->keys[index] == key) {
        leaf->values[index] = value;
        return 0;
    }

    if (leaf->key_count >= BPTREE_ORDER - 1) {
        return -1;
    }

    for (move = leaf->key_count; move > index; move--) {
        leaf->keys[move] = leaf->keys[move - 1];
        leaf->values[move] = leaf->values[move - 1];
    }

    leaf->keys[index] = key;
    leaf->values[index] = value;
    leaf->key_count++;
    return 0;
}

/* search 는 항상 루트에서 시작해서
 * "이 key 가 어느 구간에 속하는지" 를 보며 아래로 내려간다.
 * 이 함수는 마지막에 도착한 리프 노드를 찾아준다.
 */
static BPTreeNode *bptree_find_leaf(BPTree *tree, int key)
{
    BPTreeNode *node;
    int index;

    if (!tree || !tree->root) {
        return NULL;
    }

    node = tree->root;
    while (node && !node->is_leaf) {
        index = 0;
        /* 현재 key 보다 크거나 같은 경계값을 만날 때까지 오른쪽으로 이동한다.
         * 끝까지 못 찾으면 가장 오른쪽 자식으로 내려간다.
         */
        while (index < node->key_count && key >= node->keys[index]) {
            index++;
        }
        node = node->children[index];
    }

    return node;
}

/* 노드를 재귀적으로 지운다.
 * 내부 노드는 자식들을 먼저 지우고,
 * 마지막에 자기 자신을 지운다.
 */
static void bptree_destroy_node(BPTreeNode *node)
{
    int i;

    if (!node) {
        return;
    }

    if (!node->is_leaf) {
        for (i = 0; i <= node->key_count; i++) {
            bptree_destroy_node(node->children[i]);
        }
    }

    free(node);
}

/* 노드 하나를 0으로 초기화해서 만든다.
 * calloc 을 써서 key, child, value 포인터들이 모두 비어 있는 상태로 시작한다.
 */
BPTreeNode *bptree_create_node(int is_leaf)
{
    BPTreeNode *node = (BPTreeNode *)calloc(1, sizeof(BPTreeNode));

    if (!node) {
        return NULL;
    }

    node->is_leaf = is_leaf;
    return node;
}

/* 트리를 만들 때는 가장 단순한 상태로 시작한다.
 * 루트 하나만 있고, 그 루트는 곧 리프 노드다.
 */
BPTree *bptree_create(void)
{
    BPTree *tree = (BPTree *)calloc(1, sizeof(BPTree));

    if (!tree) {
        return NULL;
    }

    tree->root = bptree_create_node(1);
    if (!tree->root) {
        free(tree);
        return NULL;
    }
    tree->first_leaf = tree->root;

    return tree;
}

/* 트리 해제 함수.
 * root 아래에 달린 모든 노드를 지운 뒤 트리 구조체도 함께 지운다.
 */
void bptree_destroy(BPTree *tree)
{
    if (!tree) {
        return;
    }

    bptree_destroy_node(tree->root);
    free(tree);
}

/* search 의 실제 동작.
 * 1. 먼저 key 가 있어야 할 리프 노드를 찾는다.
 * 2. 그 리프 노드 안에서 key 를 앞에서부터 비교한다.
 * 3. 같은 key 를 찾으면 연결된 값을 돌려주고, 없으면 NULL 을 돌려준다.
 */
void *bptree_search(BPTree *tree, int key)
{
    BPTreeNode *node;
    int i;

    node = bptree_find_leaf(tree, key);
    if (!node) {
        return NULL;
    }

    for (i = 0; i < node->key_count; i++) {
        if (node->keys[i] == key) {
            return node->values[i];
        }
    }

    return NULL;
}

/* insert 는 다음 사이클에서 구현한다.
 * 현재 단계에서는:
 * 1. key 가 들어갈 리프를 찾고
 * 2. 리프가 꽉 차지 않았으면 정렬 순서를 유지하며 삽입한다.
 * split 은 아직 다음 단계에서 구현한다.
 */
int bptree_insert(BPTree *tree, int key, void *value)
{
    BPTreeNode *leaf;

    if (!tree || !tree->root) {
        return -1;
    }

    leaf = bptree_find_leaf(tree, key);
    if (!leaf) {
        return -1;
    }

    return bptree_insert_into_leaf(leaf, key, value);
}
