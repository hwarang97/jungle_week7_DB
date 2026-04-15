#include "bptree.h"

#include <stdio.h>

int main(void) {
    BPTree *tree = bptree_create();
    if (!tree) {
        return 1;
    }

    printf("B+ Tree Index Project\n");
    printf("TODO: 사이클 1에서 구조체와 search를 구현하세요.\n");

    bptree_destroy(tree);
    return 0;
}
