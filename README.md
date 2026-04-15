# B+ Tree Index Project

메모리 기반 B+ Tree 인덱스를 C로 구현하고, SQL 처리기와 연동하여 대용량 데이터 검색 성능을 비교하는 프로젝트입니다.

## 1. 팀원

| 이름 | 주요 기여 |
|---|---|
| jiun | SQL `WHERE id = ?` 인덱스 연동, 비인덱스 fallback 유지, 데모 스크립트·benchmark·검증 문서까지 정리해 최종 완성도를 높였습니다. |
| minsuk | SQL INSERT와 auto-id, `SELECT WHERE id` fast path를 안정적으로 연결하고 통합 테스트로 동작을 검증했습니다. |
| seokje | B+ Tree insert/split/search를 단계적으로 구현하고, SQL 조회에서 id 인덱스 경로와 선형 fallback을 비교 가능한 형태로 정리했습니다. |
| sein | 단순한 인덱스 캐시 기반으로 SQL id 조회 연동 방향을 실험해 팀 비교와 구조 논의에 기여했습니다. |

## 2. 빌드 및 실행 방법

```bash
make
make test
make bench
```

추가 실행 명령은 아래와 같습니다.

```bash
make test-sqlparser
make bench-sql-index
make bench-bptree-scale
make sqlparser
python3 server.py
```

- `make`: B+ Tree 실행 파일을 빌드합니다.
- `make test`: 구조 테스트와 search 테스트를 실행합니다.
- `make bench`: 기본 B+ Tree benchmark를 실행합니다.
- `make test-sqlparser`: SQL parser, executor, storage, 인덱스 연동 테스트를 실행합니다.
- `make bench-sql-index`: 기본 SQL 인덱스 benchmark를 실행합니다.
- `make bench-bptree-scale`: 기본 B+ Tree scale benchmark를 실행합니다.
- `make sqlparser`: SQL 처리기 실행 파일을 빌드합니다.
- `python3 server.py`: 브라우저에서 SQL 실행과 benchmark 버튼을 사용할 수 있는 로컬 서버를 실행합니다.

현재 B+ Tree 차수는 `BPTREE_ORDER = 4`로 설정되어 있습니다.
기본 benchmark는 빠른 데모를 위해 작은 row count와 sample 수를 사용하며, 필요하면 compile-time macro로 범위를 확장할 수 있습니다.

## 3. B+ Tree 구조 설명

### 3.1 트리 구조

![B+ Tree structure](docs/bptree-structure.svg)

- 루트 노드는 탐색이 시작되는 지점입니다.
- 내부 노드는 어느 자식으로 내려갈지 결정하는 separator key를 저장합니다.
- 리프 노드는 실제 key와 value를 저장합니다.
- 리프 노드는 `next` 포인터로 연결되어 있어 순차 탐색에 유리합니다.

### 3.2 insert와 split 과정

![B+ Tree insert and split](docs/bptree-insert-split.svg)

### 3.3 의사코드

![B+ Tree search flow](docs/bptree-search.svg)

- `SEARCH`: root에서 시작해 separator key를 비교하며 leaf까지 내려간 뒤, leaf에서 key를 찾으면 value를 반환합니다.
- `INSERT`: key가 들어갈 leaf를 찾고 정렬 상태를 유지하며 삽입합니다.
- `SPLIT`: overflow가 발생하면 leaf 또는 internal node를 둘로 나누고, separator key를 부모에 반영합니다.

## 4. 시스템 구조 (SQL 연동)

![B+ Tree system architecture](docs/bptree-system-architecture.svg)

### 4.1 흐름 요약

- `INSERT` 실행 시 레코드를 저장한 뒤, 부여된 `id` 를 B+ Tree 인덱스에 등록합니다.
- `id` 컬럼을 생략한 `INSERT` 는 auto-id를 생성한 뒤 해당 값을 인덱스 key로 사용합니다.
- `SELECT WHERE id = ?` 는 B+ Tree 인덱스를 사용합니다.
- `SELECT WHERE name = ?` 같은 비인덱스 조건은 기존 선형 탐색을 사용합니다.
- 테이블을 다시 읽을 때는 저장된 row를 기준으로 메모리 인덱스를 다시 구성합니다.

이 프로젝트의 핵심은 B+ Tree를 따로 구현하는 데서 끝나지 않고,
`SELECT WHERE id = ?` 실행 시 실제 조회 경로에서 인덱스를 사용하게 만드는 것입니다.

![Index vs linear comparison](docs/bptree-index-vs-linear.svg)

## 4.2 왜 B+ Tree 인덱스를 사용했는가

| 관점 | 내용 |
|---|---|
| 프로젝트 목표 | `WHERE id = ?` 조건에서 전체 row를 선형 탐색하지 않고 더 빠르게 조회하는 것입니다. |
| 컬럼 특성 | `id` 는 row를 잘 구분하는 key라서 인덱스 효과가 크게 나타납니다. |
| 자료구조 선택 | B+ Tree는 `=`, 범위 검색, 정렬, 순차 접근까지 넓게 대응하기 좋은 구조입니다. |
| 실무 관점 | DB 인덱스에서는 B-Tree보다 B+ Tree가 더 널리 쓰이며, 내부 노드는 탐색 경로를 줄이고 실제 데이터는 리프에 모읍니다. |

## 4.3 인덱스의 장점과 트레이드오프

| 구분 | 설명 |
|---|---|
| 읽기 장점 | `SELECT WHERE id = ?` 같은 조회는 전체 데이터를 끝까지 읽지 않고 더 적은 비교로 목표 row에 도달할 수 있습니다. |
| 규모 효과 | 데이터가 많아질수록 선형 탐색과의 성능 차이가 더 분명해집니다. |
| 쓰기 비용 | `INSERT`, `UPDATE`, `DELETE` 시에는 인덱스도 함께 갱신해야 하므로 추가 비용이 발생합니다. |
| 유지 비용 | 인덱스를 유지하기 위한 메모리와 관리 비용이 필요합니다. |
| 비효율 사례 | 선택도가 낮은 컬럼이나 자주 쓰이지 않는 컬럼은 인덱스 효과가 크지 않을 수 있습니다. |

### 4.3.1 실무 관점 포인트

**인덱스를 설계할 때 보는 기준**

| 기준 | 의미 | 예시 |
|---|---|---|
| 자주 찾는가 | 실제 조회 조건에 자주 등장하는 컬럼인지 봅니다. | `id`, `email`, 외래키 |
| 잘 구분하는가 | 값이 row를 충분히 잘 나누는지 봅니다. | `status`보다 `id`가 유리 |
| 정렬/범위/조인에 쓰이는가 | `ORDER BY`, `BETWEEN`, 조인 조건에 자주 쓰이면 가치가 큽니다. | `created_at`, foreign key |
| 유지 비용보다 이득이 큰가 | 읽기 이득이 쓰기/관리 비용보다 큰지 봅니다. | 읽기 많은 서비스에 적합 |

**인덱스 종류별 특징**

| 인덱스 | 강점 | 약점 |
|---|---|---|
| B+ Tree | 가장 범용적이며 `=`, `<`, `>`, `BETWEEN`, `ORDER BY`에 강합니다. | 쓰기 시 split과 유지 비용이 듭니다. |
| Hash Index | 정확히 같은 값 `=` 조회에 강할 수 있습니다. | 범위 검색과 정렬에는 약합니다. |
| Bitmap Index | 값 종류가 적은 컬럼의 분석성 질의에 유리할 수 있습니다. | OLTP 환경에서는 갱신 비용이 부담될 수 있습니다. |
| Full-text Index | 문자열 포함 검색에 특화되어 있습니다. | 일반적인 정렬/범위 탐색에는 적합하지 않습니다. |

**인덱스가 오히려 비효율적일 수 있는 경우**

| 경우 | 이유 |
|---|---|
| 인덱스가 너무 많을 때 | `INSERT`, `UPDATE`, `DELETE` 때마다 모두 갱신해야 합니다. |
| 선택도가 낮은 컬럼일 때 | 인덱스로 걸러도 다시 읽어야 할 row가 너무 많습니다. |
| 결과 row가 너무 많을 때 | 인덱스를 타고 본문 row를 다시 읽는 비용이 커질 수 있습니다. |
| 함수나 변형 조건을 쓸 때 | 인덱스를 제대로 타지 못할 수 있습니다. 예: `WHERE YEAR(created_at) = 2024` |

### 4.3.2 이번 프로젝트에 적용한 판단

| 판단 항목 | 이번 프로젝트의 선택 |
|---|---|
| 가장 중요한 쿼리 패턴 | `SELECT ... WHERE id = ?` |
| 인덱스 대상 컬럼 | `id` |
| 선택 이유 | `id` 는 row를 잘 구분하고, 과제 요구사항도 ID 기반 검색 최적화에 초점이 있습니다. |
| 비교 전략 | `id` 는 B+ Tree 인덱스, `name` 은 선형 탐색으로 두어 benchmark 차이를 명확히 보이도록 했습니다. |

## 4.4 요구사항 관점에서의 선택

| 요구사항 | 적용 방식 |
|---|---|
| 메모리 기반 B+ Tree 구현 | `src/bptree.c` 에서 search / insert / split을 구현했습니다. |
| SQL 처리기 연동 | `INSERT`, `SELECT WHERE id = ?` 경로에 인덱스를 실제로 연결했습니다. |
| 인덱스 효과 검증 | `id` 인덱스 조회와 `name` 선형 탐색을 benchmark로 비교했습니다. |
| 범위 제한 | 실제 DBMS의 디스크 페이지 기반 인덱스가 아니라, 과제 범위에 맞춘 메모리 기반 단순화 구현입니다. |

## 5. 벤치마크 결과

현재 `main`의 기본 `make bench-sql-index` 실행 결과를 기준으로 정리했습니다.

| 데이터 건수 | id 검색 시간 (B+ Tree) | name 검색 시간 (선형) | 배율 차이 |
|---|---:|---:|---:|
| 20 | 1.211ms | 2.276ms | 1.88x |
| 80 | 1.183ms | 1.691ms | 1.43x |
| 160 | 1.143ms | 2.545ms | 2.23x |

![SQL benchmark graph](docs/benchmark-sql-index.svg)

### 5.1 요약

- `WHERE id = ?` 는 B+ Tree 인덱스를 사용해 일정한 시간 안에 조회됩니다.
- `WHERE name = ?` 는 선형 탐색이므로 같은 조건에서도 인덱스 조회보다 느리게 측정됩니다.
- 현재 기본 benchmark는 `20 / 80 / 160` rows, `20`회 lookup 기준으로 빠르게 비교할 수 있도록 구성되어 있습니다.
- 기본 benchmark에서도 `name` 조회는 `id` 조회보다 약 `1.43배 ~ 2.23배` 느리게 나타났습니다.
- 벤치마크 실행 시 현재 차수(`order`)와 row 수가 함께 출력되도록 구성했습니다.

\* 기본 SQL benchmark는 `ID_LOOKUPS=20`, `NAME_LOOKUPS=20`, `SQL_BENCH_ROW_COUNTS=20, 80, 160` 설정을 사용합니다.  
\* fixture는 `storage_insert()`를 사용해 row를 채우고, 이후 `WHERE id = ?` 와 `WHERE name = ?` 조회를 반복 측정합니다.

### 5.2 벤치마크 실행 예시

```bash
make bench-sql-index
make bench-bptree-scale
```

더 큰 benchmark가 필요하면 `benchmark/bench_sql_index.c`와 `benchmark/bench_bptree_scale.c`의 compile-time macro를 조정해 row 수와 sample 수를 확장할 수 있습니다.

### 5.3 B+ Tree 자체 확장성 확인

현재 `main`의 기본 `make bench-bptree-scale` 실행 결과는 아래와 같습니다.

| 데이터 건수 | B+ Tree insert | B+ Tree search | linear search |
|---|---:|---:|---:|
| 1,000 | 0.072ms | 0.050ms | 0.167ms |
| 5,000 | 0.360ms | 0.072ms | 0.809ms |
| 10,000 | 0.770ms | 0.075ms | 1.543ms |

![B+ Tree scale benchmark graph](docs/benchmark-bptree-scale.svg)

### 5.4 벤치마크 해석

- `bptree search`는 row 수가 커져도 매우 낮은 시간으로 유지됩니다.
- `linear search`는 row 수 증가에 따라 더 가파르게 증가합니다.
- `bptree insert`는 row 저장과 트리 갱신이 함께 일어나므로 search보다 비용이 크지만, 현재 기본 benchmark 범위에서는 완만하게 증가합니다.

## 6. 테스트 결과

### 6.1 통과한 테스트 종류

- B+ Tree 구조 테스트
- search 테스트
- SQL parser / executor 테스트
- storage insert / delete / update / select 테스트
- SQL 인덱스 연동 테스트
- auto-id 생성 및 증가 테스트
- `WHERE id = ?` fast path / 비인덱스 fallback 테스트

### 6.2 실행 결과 예시

```text
Structure tests passed! (6/6)
Search tests passed! (11/11)
197 passed, 0 failed
[EXECUTOR TESTS] passed
All INSERT storage tests passed.
All DELETE storage tests passed.
All UPDATE storage tests passed.
[ROWSET TESTS] 33 passed, 0 failed
[SQL INDEX INTEGRATION] passed
```

### 6.3 추가 검증 명령

```bash
make test
make test-sqlparser
make test_sql_index_integration
```

### 6.4 테스트 요약

- B+ Tree 구조 테스트: `6/6 PASS`
- B+ Tree search 테스트: `11/11 PASS`
- parser 테스트: `197 passed, 0 failed`
- RowSet / storage select 테스트: `33 passed, 0 failed`
- SQL 인덱스 통합 테스트: `passed`
- auto-id 생략 `INSERT` 테스트: `passed`
- auto-id 연속 증가 테스트: `passed`

## 7. 프로젝트 구조

```text
jungle_week7_DB/
├── src/
│   ├── bptree.h
│   ├── bptree.c
│   ├── bptree_main.c
│   ├── parser.c
│   ├── executor.c
│   ├── storage.c
│   └── main.c
├── test/
│   ├── test_structure.c
│   └── test_search.c
├── tests/
│   ├── test_parser.c
│   ├── test_executor.c
│   ├── test_storage_insert.c
│   ├── test_storage_delete.c
│   ├── test_storage_update.c
│   ├── test_storage_select_result.c
│   └── test_sql_index_integration.c
├── benchmark/
│   ├── bench_search.c
│   ├── bench_sql_index.c
│   └── bench_bptree_scale.c
├── index.html
├── server.py
├── docs/
│   ├── bptree-search-simulator.html
│   ├── bptree-structure.svg
│   ├── bptree-insert-split.svg
│   ├── bptree-search.svg
│   ├── bptree-system-architecture.svg
│   ├── bptree-index-vs-linear.svg
│   ├── benchmark-sql-index.svg
│   └── benchmark-bptree-scale.svg
├── Makefile
└── README.md
```

### 7.1 주요 파일 역할

- `src/bptree.h`: B+ Tree 구조체와 함수 선언을 담고 있습니다.
- `src/bptree.c`: search, insert, split의 핵심 구현을 담고 있습니다.
- `src/storage.c`: SQL 처리기와 B+ Tree 인덱스 연동 로직을 담고 있습니다.
- `test/test_structure.c`: B+ Tree 구조 불변식을 검증합니다.
- `test/test_search.c`: search 동작을 검증합니다.
- `tests/test_storage_insert.c`: storage insert와 auto-id 동작을 검증합니다.
- `tests/test_storage_select_result.c`: SQL `SELECT` 결과를 검증합니다.
- `tests/test_sql_index_integration.c`: SQL 인덱스 연동을 검증합니다.
- `benchmark/bench_sql_index.c`: `id` 인덱스 조회와 비인덱스 조회 성능을 비교합니다.
- `benchmark/bench_bptree_scale.c`: 데이터 증가에 따른 B+ Tree 성능을 확인합니다.
- `index.html`: SQL 실행과 benchmark 버튼을 제공하는 브라우저 UI입니다.
- `server.py`: 브라우저 요청을 받아 `sqlparser`와 benchmark 타깃을 실행하는 로컬 HTTP 서버입니다.
- `docs/bptree-search-simulator.html`: B+ Tree 탐색 흐름을 시각적으로 확인하는 보조 자료입니다.

## 8. 한계 및 개선 방향

- 현재 구현은 실제 DBMS의 디스크 기반 B+ Tree가 아닌 메모리 기반 단순화 버전입니다.
- 현재 인덱스는 `id` 기준 단일 equality 조회에 초점을 맞추고 있습니다.
- 현재 기본 benchmark는 빠른 데모 중심의 작은 데이터 범위를 사용하며, 대규모 측정은 compile-time macro 조정이 필요합니다.
- 향후 범위 검색, delete, 다중 인덱스, 디스크 페이지 관리로 확장할 수 있습니다.
