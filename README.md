# B+ Tree Index

## 팀원
- jiun
- minsuk
- seokje
- sein

## 빌드 및 실행

```bash
make
./bptree
```

```bash
make test
make bench
```

기본 실행은 B+ Tree 프로젝트 시작 메시지를 출력합니다. SQL 파일 경로를 넘기면
기존 SQL 처리기 코드를 통해 파일 단위 실행도 가능합니다.

```bash
./bptree query.sql
```

## 프로젝트 구조

```text
bptree-index/
├── src/
│   ├── bptree.h
│   ├── bptree.c
│   ├── sql_processor.h
│   ├── sql_processor.c
│   └── main.c
├── test/
│   └── test_bptree.c
├── benchmark/
│   └── bench_search.c
├── Makefile
├── .gitignore
├── README.md
└── WORKFLOW.md
```

- `src/bptree.h`: B+ Tree 노드/트리 구조체와 함수 선언
- `src/bptree.c`: B+ Tree 기본 생성/해제와 사이클 구현 포인트
- `src/sql_processor.h`: 기존 SQL 처리기 래퍼 인터페이스
- `src/sql_processor.c`: 기존 parser/executor/storage 코드를 묶는 진입 래퍼
- `src/main.c`: `bptree` 실행파일 진입점
- `test/test_bptree.c`: 사이클 중 늘려갈 단위 테스트 뼈대
- `benchmark/bench_search.c`: 수렴 단계에서 확장할 검색 벤치마크 뼈대

## B+ Tree 설명
<!-- TODO: 정렬 Phase에서 작성 -->

## 벤치마크 결과
<!-- TODO: 수렴 Phase에서 작성 -->

## 테스트 결과
<!-- TODO: 수렴 Phase에서 작성 -->
