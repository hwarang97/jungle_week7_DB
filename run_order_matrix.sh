#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

ORDERS=(3 4 32 128)
BASE_CFLAGS="-Wall -Wextra -O2 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I./include -I./src"

printf '\033[1m\033[36m[order-matrix]\033[0m BPTREE_ORDER 별 코어 B+ Tree 테스트를 다시 빌드해서 확인합니다.\n'
printf '%-10s %-24s %-24s\n' "order" "order-smoke" "search"

for order in "${ORDERS[@]}"; do
    echo "[order-matrix] rebuilding with BPTREE_ORDER=${order}"
    make clean >/dev/null

    make test_order_matrix CFLAGS="${BASE_CFLAGS} -DBPTREE_ORDER=${order}" >/dev/null
    make test_search CFLAGS="${BASE_CFLAGS} -DBPTREE_ORDER=${order}" >/dev/null
    structure_output=$(./test_order_matrix 2>&1)
    search_output=$(./test_search 2>&1)

    structure_summary=$(printf '%s\n' "$structure_output" | tail -n 1)
    search_summary=$(printf '%s\n' "$search_output" | tail -n 1)

    printf '%-10s %-24s %-24s\n' \
        "$order" \
        "${structure_summary:0:24}" \
        "${search_summary:0:24}"
done

printf '\n\033[1m\033[36m[hint]\033[0m 상세 출력이 필요하면 예시처럼 개별 실행하세요.\n'
printf '  make test_order_matrix CFLAGS="%s -DBPTREE_ORDER=32" && ./test_order_matrix\n' "$BASE_CFLAGS"
printf '  make test_search CFLAGS="%s -DBPTREE_ORDER=32" && ./test_search\n' "$BASE_CFLAGS"
