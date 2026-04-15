#ifndef BPTREE_H
#define BPTREE_H

#define BPTREE_ORDER 4

/* B+ Tree 의 한 노드.
 * 내부 노드와 리프 노드가 같은 구조를 쓰고,
 * is_leaf 값으로 현재 노드 종류를 구분한다.
 */
typedef struct BPTreeNode
{
    int is_leaf;                               /* 1이면 리프 노드, 0이면 내부 노드 */
    int key_count;                             /* 현재 노드에 실제로 들어 있는 key 개수 */
    int keys[BPTREE_ORDER - 1];                /* key 들은 항상 작은 값부터 차례대로 저장 */
    struct BPTreeNode *children[BPTREE_ORDER]; /* 내부 노드일 때 아래 자식들을 가리킴 */
    void *values[BPTREE_ORDER - 1];            /* 리프 노드일 때 key 에 대응하는 값 */
    struct BPTreeNode *parent;                 /* split 때 위로 올라가기 위해 부모를 기억 */
    struct BPTreeNode *next;                   /* 리프 노드끼리 오른쪽으로 연결 */
} BPTreeNode;

/* 트리 전체를 나타내는 구조체.
 * search 는 root 에서 시작하고,
 * 나중에 range scan 이 필요할 때는 first_leaf 부터 순서대로 볼 수 있다.
 */
typedef struct BPTree
{
    BPTreeNode *root;       /* 탐색을 시작하는 가장 위 노드 */
    BPTreeNode *first_leaf; /* 가장 왼쪽 리프 노드 */
} BPTree;

/* 사이클 1에서 구조체를 확정하고, 사이클 2에서 insert/split 을 붙인다. */

/* 빈 트리를 만든다. 처음에는 루트 하나만 있는 리프 노드 상태다. */
BPTree *bptree_create(void);

/* 트리와 그 안에 있는 모든 노드를 함께 해제한다. */
void bptree_destroy(BPTree *tree);

/* 노드 하나를 새로 만든다. is_leaf 값으로 리프/내부 노드를 정한다. */
BPTreeNode *bptree_create_node(int is_leaf);

/* key 를 찾아서 리프 노드 안의 값을 돌려준다. 없으면 NULL 을 돌려준다. */
void *bptree_search(BPTree *tree, int key);

/* 사이클 2에서 구현할 삽입 함수. */
int bptree_insert(BPTree *tree, int key, void *value);

#endif
