# Cycle 3 SQL Demo

이 데모는 사이클 3에서 연결한 "SQL `WHERE id = ?` -> B+ Tree 인덱스 조회"를
빠르게 보여주기 위한 예시입니다.

## 실행 명령

```bash
make sqlparser
./sqlparser docs/cycle3-index-demo.sql
```

## 데모 포인트

1. `SELECT * FROM users_idx_demo WHERE id = 2;`
   `id` 컬럼 단일 equality 조회라서 B+ Tree 인덱스 경로를 탑니다.

2. `SELECT * FROM users_idx_demo WHERE name = 'minsuk';`
   `name`은 현재 인덱스를 붙이지 않았으므로 기존 선형 탐색 경로를 탑니다.

## 발표 때 말할 문장

- `id` 조회는 인덱스를 탄다.
- `name` 조회는 아직 인덱스가 없어서 기존 스캔 경로를 탄다.
- 즉, B+ Tree가 SQL 처리기 안의 실제 조회 경로에 연결됐다.
