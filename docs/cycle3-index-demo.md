# Cycle 3 SQL Demo

이 데모는 사이클 3에서 연결한 "SQL `WHERE id = ?` -> B+ Tree 인덱스 조회"를
빠르게 보여주기 위한 예시입니다.

## 실행 명령

가장 간단한 방법:

```bash
make verify-cycle3
```

이 명령은 아래 4가지를 한 번에 확인합니다.

1. B+ Tree / search / edge-case 테스트
2. SQL parser + executor + storage 테스트
3. SQL 인덱스 benchmark
4. 실제 cycle 3 데모 쿼리 실행

데모만 빠르게 보고 싶다면:

```bash
./run_cycle3_index_demo.sh
```

수동으로 실행하려면:

```bash
make sqlparser
./sqlparser docs/cycle3-index-demo.sql
```

조회 경로까지 같이 보고 싶으면:

```bash
STORAGE_TRACE_SELECT=1 ./sqlparser docs/cycle3-index-demo.sql
```

간단한 속도 비교를 보고 싶으면:

```bash
make bench-sql-index
```

다른 `order`로 비교해 보고 싶으면:

```bash
make clean
make bench-sql-index CFLAGS="-Wall -Wextra -O2 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I./include -I./src -DBPTREE_ORDER=32"
```

## 데모 포인트

1. `SELECT * FROM users_idx_demo WHERE id = 2;`
   `id` 컬럼 단일 equality 조회라서 B+ Tree 인덱스 경로를 탑니다.

2. `SELECT * FROM users_idx_demo WHERE name = 'minsuk';`
   `name`은 현재 인덱스를 붙이지 않았으므로 기존 선형 탐색 경로를 탑니다.

3. `make bench-sql-index`
   현재 `BPTREE_ORDER`와 데이터 건수별로 `id` 조회 평균 시간,
   `name` 조회 평균 시간을 콘솔에 출력합니다.

4. `make verify-cycle3`
   사이클 3 범위가 실제로 통과하는지 팀원이 한 줄 명령으로 재검증할 수 있습니다.

## 완료 기준

- `INSERT` 후 `SELECT WHERE id = ?` 가 정확한 1건을 반환한다.
- 존재하지 않는 `id` 조회는 빈 결과를 반환한다.
- 인덱스가 없는 컬럼 조회는 기존 선형 탐색으로 유지된다.
- benchmark 에서 `id index` 와 `name scan` 시간을 같은 조건에서 비교할 수 있다.
- 데모 실행 시 어떤 조회가 `indexed-bptree`, 어떤 조회가 `linear-scan` 인지 콘솔에서 보인다.

## 발표 때 말할 문장

- `id` 조회는 인덱스를 탄다.
- `name` 조회는 아직 인덱스가 없어서 기존 스캔 경로를 탄다.
- benchmark 로 두 경로의 차이를 숫자로 보여줄 수 있다.
- 즉, B+ Tree가 SQL 처리기 안의 실제 조회 경로에 연결됐다.
