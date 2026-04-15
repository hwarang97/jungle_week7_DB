#include <limits.h>
#include <stdio.h>

#include "../src/bptree.h"

typedef enum TestStatus {
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP
} TestStatus;

typedef struct TestResult {
    TestStatus status;
    const char *detail;
} TestResult;

typedef struct TestCase {
    const char *id;
    const char *name;
    TestResult (*fn)(void);
} TestCase;

#define PASS(msg) ((TestResult){TEST_PASS, msg})
#define FAIL(msg) ((TestResult){TEST_FAIL, msg})
#define SKIP(msg) ((TestResult){TEST_SKIP, msg})

static TestResult tc01_empty_tree_search(void)
{
    BPTree *tree = bptree_create();
    void *result;

    if (!tree) {
        return FAIL("트리 생성 실패");
    }

    result = bptree_search(tree, 1);
    bptree_destroy(tree);

    if (result != NULL) {
        return FAIL("빈 트리 검색은 NULL 이어야 함");
    }
    return PASS("빈 트리에서 NULL 반환");
}

static TestResult tc02_single_insert_search(void)
{
    BPTree *tree = bptree_create();
    static const char value[] = "hello";

    if (!tree) {
        return FAIL("트리 생성 실패");
    }
    if (bptree_insert(tree, 5, (void *)value) != 0) {
        bptree_destroy(tree);
        return FAIL("단일 삽입 실패");
    }
    if (bptree_search(tree, 5) != value) {
        bptree_destroy(tree);
        return FAIL("삽입한 key 를 검색하지 못함");
    }

    bptree_destroy(tree);
    return PASS("삽입 후 검색 성공");
}

static TestResult tc03_missing_key_after_insert(void)
{
    BPTree *tree = bptree_create();
    static const char value[] = "hello";

    if (!tree) {
        return FAIL("트리 생성 실패");
    }
    if (bptree_insert(tree, 5, (void *)value) != 0) {
        bptree_destroy(tree);
        return FAIL("삽입 실패");
    }
    if (bptree_search(tree, 99) != NULL) {
        bptree_destroy(tree);
        return FAIL("없는 key 검색은 NULL 이어야 함");
    }

    bptree_destroy(tree);
    return PASS("없는 key 검색 성공");
}

static TestResult tc04_first_leaf_split(void)
{
    BPTree *tree = bptree_create();
    int values[BPTREE_ORDER];
    int i;

    if (!tree) {
        return FAIL("트리 생성 실패");
    }

    for (i = 0; i < BPTREE_ORDER; i++) {
        values[i] = i + 1;
        if (bptree_insert(tree, i + 1, &values[i]) != 0) {
            bptree_destroy(tree);
            return FAIL("첫 리프 split 유도 삽입 실패");
        }
    }

    if (tree->root->is_leaf) {
        bptree_destroy(tree);
        return FAIL("split 후 루트는 내부 노드여야 함");
    }
    if (tree->root->children[0] == NULL || tree->root->children[1] == NULL) {
        bptree_destroy(tree);
        return FAIL("split 후 두 리프가 있어야 함");
    }
    if (tree->first_leaf != tree->root->children[0]) {
        bptree_destroy(tree);
        return FAIL("first_leaf 가 왼쪽 리프를 가리켜야 함");
    }

    bptree_destroy(tree);
    return PASS("루트 리프 split 성공");
}

static TestResult tc05_internal_root_split(void)
{
    return SKIP("현재 구현은 루트 리프 split 까지만 지원하고 내부 노드 split 은 미구현");
}

static TestResult tc06_cascading_split(void)
{
    return SKIP("연쇄 split 은 부모 삽입과 내부 split 구현 후 활성화 예정");
}

static TestResult tc07_reverse_insert(void)
{
    return SKIP("역순 대량 삽입은 부모가 있는 리프 split 이 필요함");
}

static TestResult tc08_random_insert(void)
{
    return SKIP("랜덤 순서 대량 삽입은 일반 split 경로 구현 후 활성화 예정");
}

static TestResult tc09_hotspot_insert(void)
{
    return SKIP("오른쪽 리프 연쇄 split 패턴은 아직 미지원");
}

static TestResult tc10_duplicate_key_policy(void)
{
    BPTree *tree = bptree_create();
    static const char first[] = "first";
    static const char second[] = "second";

    if (!tree) {
        return FAIL("트리 생성 실패");
    }
    if (bptree_insert(tree, 5, (void *)first) != 0) {
        bptree_destroy(tree);
        return FAIL("첫 삽입 실패");
    }
    if (bptree_insert(tree, 5, (void *)second) != 0) {
        bptree_destroy(tree);
        return FAIL("중복 삽입 실패");
    }
    if (bptree_search(tree, 5) != second) {
        bptree_destroy(tree);
        return FAIL("현재 정책은 덮어쓰기여야 함");
    }

    bptree_destroy(tree);
    return PASS("중복 key 는 덮어쓰기 정책으로 동작");
}

static TestResult tc11_duplicate_key_massive(void)
{
    BPTree *tree = bptree_create();
    int values[100];
    int i;

    if (!tree) {
        return FAIL("트리 생성 실패");
    }

    for (i = 0; i < 100; i++) {
        values[i] = i;
        if (bptree_insert(tree, 1, &values[i]) != 0) {
            bptree_destroy(tree);
            return FAIL("중복 대량 삽입 중 실패");
        }
    }

    if (tree->root->is_leaf && tree->root->key_count != 1) {
        bptree_destroy(tree);
        return FAIL("중복 삽입 후 key_count 는 1 이어야 함");
    }
    if (bptree_search(tree, 1) != &values[99]) {
        bptree_destroy(tree);
        return FAIL("마지막 값으로 갱신되지 않음");
    }

    bptree_destroy(tree);
    return PASS("중복 key 대량 삽입도 일관된 덮어쓰기");
}

static TestResult tc12_min_order(void)
{
    return SKIP("현재 구현은 BPTREE_ORDER 가 컴파일 타임 상수라 order=3 전환 테스트를 지원하지 않음");
}

static TestResult tc13_large_order(void)
{
    return SKIP("현재 구현은 가변 order 를 지원하지 않음");
}

static TestResult tc14_zero_key(void)
{
    BPTree *tree = bptree_create();
    static const char value[] = "zero";

    if (!tree) {
        return FAIL("트리 생성 실패");
    }
    if (bptree_insert(tree, 0, (void *)value) != 0) {
        bptree_destroy(tree);
        return FAIL("0 삽입 실패");
    }
    if (bptree_search(tree, 0) != value) {
        bptree_destroy(tree);
        return FAIL("0 검색 실패");
    }

    bptree_destroy(tree);
    return PASS("0 key 처리 성공");
}

static TestResult tc15_int_max_key(void)
{
    BPTree *tree = bptree_create();
    static const char value[] = "max_int";

    if (!tree) {
        return FAIL("트리 생성 실패");
    }
    if (bptree_insert(tree, INT_MAX, (void *)value) != 0) {
        bptree_destroy(tree);
        return FAIL("INT_MAX 삽입 실패");
    }
    if (bptree_search(tree, INT_MAX) != value) {
        bptree_destroy(tree);
        return FAIL("INT_MAX 검색 실패");
    }

    bptree_destroy(tree);
    return PASS("INT_MAX key 처리 성공");
}

static TestResult tc16_negative_key(void)
{
    BPTree *tree = bptree_create();
    static const char value[] = "negative";

    if (!tree) {
        return FAIL("트리 생성 실패");
    }
    if (bptree_insert(tree, -1, (void *)value) != 0) {
        bptree_destroy(tree);
        return FAIL("음수 key 삽입 실패");
    }
    if (bptree_search(tree, -1) != value) {
        bptree_destroy(tree);
        return FAIL("음수 key 검색 실패");
    }

    bptree_destroy(tree);
    return PASS("현재 구현은 음수 key 를 허용");
}

static TestResult tc17_massive_sequential_insert(void)
{
    return SKIP("100만 건 삽입과 랜덤 검색은 전체 split 구현 및 benchmark 경로가 필요함");
}

static TestResult tc18_massive_random_insert(void)
{
    return SKIP("랜덤 100만 건 삽입은 전체 split 구현과 benchmark 경로가 필요함");
}

static TestResult tc19_full_search_after_10000_insert(void)
{
    return SKIP("10,000건 전수 삽입은 현재 split 범위를 넘어감");
}

static TestResult tc20_tree_height_check(void)
{
    return SKIP("높이 검증은 연쇄 split 과 tree_height 활용 가능한 전체 삽입 구현 후 활성화 예정");
}

static TestResult tc21_leaf_link_integrity(void)
{
    BPTree *tree = bptree_create();
    int values[BPTREE_ORDER];
    BPTreeNode *leaf;
    int i;

    if (!tree) {
        return FAIL("트리 생성 실패");
    }

    for (i = 0; i < BPTREE_ORDER; i++) {
      values[i] = i + 1;
      if (bptree_insert(tree, i + 1, &values[i]) != 0) {
          bptree_destroy(tree);
          return FAIL("루트 리프 split 준비 삽입 실패");
      }
    }

    leaf = tree->first_leaf;
    if (!leaf || leaf->next == NULL) {
        bptree_destroy(tree);
        return FAIL("split 후 next 링크가 있어야 함");
    }
    if (leaf->keys[0] != 1 || leaf->next->keys[0] != 3) {
        bptree_destroy(tree);
        return FAIL("현재 split 결과의 리프 순서가 기대와 다름");
    }

    bptree_destroy(tree);
    return PASS("현재 구현 범위에서는 첫 split 후 리프 링크 유지");
}

static TestResult tc22_internal_key_order(void)
{
    return SKIP("랜덤 삽입 기반 내부 노드 정렬 검증은 내부 split 구현 후 활성화 예정");
}

static TestResult tc23_sql_insert_select_id(void)
{
    return SKIP("B+ Tree 와 SQL INSERT/SELECT id 연동은 아직 구현되지 않음");
}

static TestResult tc24_sql_select_missing_id(void)
{
    return SKIP("B+ Tree 와 SQL SELECT id miss 처리 연동은 아직 구현되지 않음");
}

static TestResult tc25_sql_select_non_index_field(void)
{
    return SKIP("인덱스 없는 필드에 대한 SQL 선형 탐색 연동은 아직 구현되지 않음");
}

static TestResult tc26_sql_performance_compare(void)
{
    return SKIP("SQL 연동 성능 비교는 인덱스 연결과 benchmark 구현 후 활성화 예정");
}

static TestResult tc27_create_insert_destroy_memory(void)
{
    return SKIP("100건 삽입 후 destroy 메모리 검증은 전체 삽입 구현과 valgrind 환경이 필요함");
}

static TestResult tc28_empty_tree_destroy(void)
{
    BPTree *tree = bptree_create();

    if (!tree) {
        return FAIL("트리 생성 실패");
    }
    bptree_destroy(tree);
    return PASS("빈 트리 destroy 호출 성공");
}

static TestResult tc29_insert_search_interleaving(void)
{
    return SKIP("삽입-검색 교차 10,000회는 전체 split 구현 후 활성화 예정");
}

static TestResult tc30_multi_order_matrix(void)
{
    return SKIP("현재 구현은 다양한 order 값을 런타임에 바꾸는 테스트를 지원하지 않음");
}

static void run_case(const TestCase *test_case, int index, int total,
                     int *pass_count, int *fail_count, int *skip_count)
{
    TestResult result = test_case->fn();
    const char *label = "PASS";

    if (result.status == TEST_FAIL) {
        label = "FAIL";
        (*fail_count)++;
    } else if (result.status == TEST_SKIP) {
        label = "SKIP";
        (*skip_count)++;
    } else {
        (*pass_count)++;
    }

    printf("[EDGE %02d/%02d] %s %s\n", index, total, test_case->id, test_case->name);
    printf("  -> %s: %s\n", label, result.detail);
}

int main(void)
{
    TestCase tests[] = {
        {"TC-01", "빈 트리에서 검색", tc01_empty_tree_search},
        {"TC-02", "단일 삽입 후 검색", tc02_single_insert_search},
        {"TC-03", "삽입 후 존재하지 않는 키 검색", tc03_missing_key_after_insert},
        {"TC-04", "리프 노드 첫 번째 분할", tc04_first_leaf_split},
        {"TC-05", "내부 노드 분할 (루트 분할)", tc05_internal_root_split},
        {"TC-06", "연쇄 분할", tc06_cascading_split},
        {"TC-07", "역순 삽입", tc07_reverse_insert},
        {"TC-08", "랜덤 순서 삽입", tc08_random_insert},
        {"TC-09", "같은 위치에 집중 삽입", tc09_hotspot_insert},
        {"TC-10", "동일 키 삽입 정책", tc10_duplicate_key_policy},
        {"TC-11", "중복 키 대량 삽입", tc11_duplicate_key_massive},
        {"TC-12", "order 최솟값", tc12_min_order},
        {"TC-13", "큰 order 값", tc13_large_order},
        {"TC-14", "키 값이 0", tc14_zero_key},
        {"TC-15", "키 값이 매우 큼", tc15_int_max_key},
        {"TC-16", "키 값이 음수", tc16_negative_key},
        {"TC-17", "순차 대량 삽입", tc17_massive_sequential_insert},
        {"TC-18", "랜덤 대량 삽입", tc18_massive_random_insert},
        {"TC-19", "대량 삽입 후 전수 검색", tc19_full_search_after_10000_insert},
        {"TC-20", "삽입 후 트리 높이 검증", tc20_tree_height_check},
        {"TC-21", "리프 링크 무결성", tc21_leaf_link_integrity},
        {"TC-22", "내부 노드 키 정렬 검증", tc22_internal_key_order},
        {"TC-23", "INSERT 후 SELECT WHERE id = ?", tc23_sql_insert_select_id},
        {"TC-24", "SELECT WHERE id = ? (존재하지 않는 ID)", tc24_sql_select_missing_id},
        {"TC-25", "SELECT WHERE name = ? (인덱스 없는 필드)", tc25_sql_select_non_index_field},
        {"TC-26", "대량 INSERT 후 SELECT 성능 비교", tc26_sql_performance_compare},
        {"TC-27", "트리 생성 후 파괴", tc27_create_insert_destroy_memory},
        {"TC-28", "빈 트리 파괴", tc28_empty_tree_destroy},
        {"TC-29", "삽입-검색 교차 반복", tc29_insert_search_interleaving},
        {"TC-30", "다양한 order에서 동일 테스트", tc30_multi_order_matrix},
    };
    int total = (int)(sizeof(tests) / sizeof(tests[0]));
    int pass_count = 0;
    int fail_count = 0;
    int skip_count = 0;
    int i;

    for (i = 0; i < total; i++) {
        run_case(&tests[i], i + 1, total, &pass_count, &fail_count, &skip_count);
    }

    printf("\nEdge-case summary: PASS=%d, FAIL=%d, SKIP=%d, TOTAL=%d\n",
           pass_count, fail_count, skip_count, total);

    return fail_count == 0 ? 0 : 1;
}
