#include "bptree.h"
#include "sql_processor.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    BPTree *tree = bptree_create();

    if (!tree) {
        fprintf(stderr, "failed to initialize B+ Tree\n");
        return 1;
    }

    if (argc == 1) {
        printf("B+ Tree Index Project\n");
        bptree_destroy(tree);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0) {
        printf("usage: %s [query.sql]\n", argv[0]);
        printf("인자를 주지 않으면 프로젝트 시작 메시지를 출력합니다.\n");
        printf("SQL 파일을 주면 기존 SQL 처리기를 통해 실행합니다.\n");
        bptree_destroy(tree);
        return 0;
    }

    if (sql_processor_run_file(argv[1], (SQLProcessorOptions){0}) != 0) {
        fprintf(stderr, "failed to run SQL file: %s\n", argv[1]);
        bptree_destroy(tree);
        return 1;
    }

    bptree_destroy(tree);
    return 0;
}
