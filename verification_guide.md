# 필수 미충족 항목 및 INT_MAX 테스트 검증 가이드

이 문서는 현재 과제에서 중요하게 보셨던 항목들 중, 특히 이전에 `필수 + 미충족`으로 정리했던 사항들을 실제로 어떻게 검증할지 정리한 문서입니다.

아래 항목들은 단순 코드 열람만으로 끝나지 않고, 일부는 반드시 빌드와 실행 결과까지 확인해야 최종 판정할 수 있습니다.

참고로 이 프로젝트의 `make` 타깃 중 일부는 `.PHONY`가 아니라서, 실행 파일이 이미 존재하면 `up to date`만 출력하고 실제 실행을 건너뛸 수 있습니다. 그런 경우에는 생성된 실행 파일을 직접 실행해서 결과를 확인하는 것이 안전합니다.

또 하나 중요한 점은, 현재 parser는 코드상 `INSERT INTO table (col, col, ...) VALUES (...)` 형태만 직접 지원합니다. 즉 `INSERT INTO table VALUES (...)` 형태는 storage 레벨에서는 검증할 수 있어도, 현재 SQL parser 경로에서는 완전한 end-to-end 검증 항목으로 남을 수 있습니다.

## 1. 레코드 추가 시 ID가 자동 부여되는지 검증

### 검증 목적
- `INSERT` 수행 시 사용자가 `id` 값을 직접 주지 않아도 저장 레코드에 ID가 자동으로 들어가는지 확인합니다.

### 확인 방법
1. 자동 ID 관련 단위 테스트 실행

```powershell
make test_storage_insert
```

`make`가 실행 대신 `up to date`만 출력하면 아래처럼 직접 실행합니다.

```powershell
./test_storage_insert
```

2. 출력에서 아래 테스트가 통과하는지 확인합니다.
- `insert auto id when id omitted`
- `insert auto id with named columns`

### 충족 판정 기준
- `tests/test_storage_insert.c`의 자동 ID 관련 테스트가 모두 통과해야 합니다.
- 삽입된 첫 번째 레코드부터 `id` 필드가 비어 있지 않고 실제 정수 값으로 저장되어야 합니다.

### 부분충족 또는 미충족으로 보는 경우
- 특정 형태의 `INSERT`에서만 자동 ID가 들어가고 다른 형태에서는 실패하는 경우
- 자동 ID는 생성되지만 레코드 저장 결과에 반영되지 않는 경우

## 2. 자동 부여 ID가 모든 INSERT 형태에서 일관되게 동작하는지 검증

### 검증 목적
- 컬럼 지정 INSERT와 컬럼 미지정 INSERT 모두에서 자동 ID 정책이 동일하게 적용되는지 확인합니다.

### 확인 방법
1. 저장소 레벨 테스트 실행

```powershell
make test_storage_insert
```

또는

```powershell
./test_storage_insert
```

2. 특히 아래 두 경로를 함께 봅니다.
- `id`를 생략한 컬럼 지정 INSERT
- `columns == NULL`인 일반 VALUES 삽입 경로

3. SQL parser 포함 end-to-end 검증 여부는 별도로 판단합니다.

```powershell
make sqlparser
```

### 매우 중요한 해석 기준
- 현재 parser 코드를 보면 `INSERT INTO table (col, col, ...) VALUES (...)`만 지원합니다.
- 따라서 `INSERT INTO users VALUES ('bob', 21);` 같은 형태는 storage 레벨에서는 검증 가능해도, 현재 SQL parser 경로에서는 그대로 입력해 통과하는지 별도 확인이 필요합니다.
- 즉 `test_storage_insert`가 통과해도, 이 항목은 자동으로 "완전 충족"이 되지 않을 수 있습니다.

### 충족 판정 기준
- 저장소 레벨에서 두 INSERT 형태 모두 자동 ID가 붙어야 합니다.
- 생성된 ID 규칙과 저장 결과가 동일해야 합니다.
- SQL 입력 경로에서도 두 형태가 모두 정상 동작해야 완전 충족으로 볼 수 있습니다.

### 부분충족 또는 미충족으로 보는 경우
- `storage_insert()` 직접 호출에서는 되지만 SQL parser 경로에서는 특정 구문이 막히는 경우
- 컬럼 지정 INSERT에서는 되지만 일반 VALUES 형태에서는 자동 ID가 붙지 않는 경우
- parser가 일반 VALUES 형태를 아직 지원하지 않아 end-to-end 검증이 불가능한 경우

## 3. 자동 ID가 연속 증가 규칙을 따르는지 검증

### 검증 목적
- 자동 생성되는 ID가 중복 없이 증가하는지, 삽입 순서대로 `1, 2, 3 ...` 혹은 내부 정책에 맞는 연속 증가를 보이는지 확인합니다.

### 확인 방법
1. 자동 증가 테스트 실행

```powershell
make test_storage_insert
```

또는

```powershell
./test_storage_insert
```

2. 출력에서 아래 테스트가 통과하는지 확인합니다.
- `insert auto id increments across rows`

3. 필요하면 CSV 결과까지 직접 확인합니다.
- 첫 번째 INSERT의 ID
- 두 번째 INSERT의 ID
- 세 번째 INSERT의 ID

### 충족 판정 기준
- 연속 삽입에서 ID가 중복 없이 증가해야 합니다.
- 중간에 누락되거나 역행하지 않아야 합니다.

### 부분충족 또는 미충족으로 보는 경우
- 일부 삽입 경로에서만 증가 규칙이 유지되는 경우
- 컬럼 지정 여부에 따라 증가 규칙이 달라지는 경우
- 같은 세션에서 중복 ID가 생성되는 경우

## 4. 최소 1,000,000개 이상의 레코드를 Insert하는지 검증

### 검증 목적
- 과제 본문에 명시된 최소 요구치인 100만 건 이상 삽입이 실제로 수행되는지 확인합니다.

### 확인 방법
1. SQL 인덱스 벤치마크 실행

```powershell
make bench-sql-index
```

`make`가 실제 실행을 생략하면 아래처럼 직접 실행합니다.

```powershell
./bench_sql_index
```

2. 출력에서 각 케이스를 확인합니다.
- `rows=10000`
- `rows=100000`
- `rows=1000000`

3. 특히 시작 메시지에서 아래 문구를 확인합니다.
- `fixture uses actual storage_insert() with auto-generated ids`

### 충족 판정 기준
- `rows=1000000` 케이스까지 실제 실행이 완료되어야 합니다.
- 실행 도중 중단되지 않고 벤치마크 결과가 끝까지 출력되어야 합니다.
- fixture가 실제 `storage_insert()` 경로를 사용해야 합니다.

### 부분충족 또는 미충족으로 보는 경우
- 코드상으로만 100만 건이 설정되어 있고 실제 실행을 끝까지 확인하지 못한 경우
- 10만 건 이하까지만 완료되고 100만 건은 실패하거나 중단되는 경우
- 100만 건 데이터가 실제 INSERT 경로가 아니라 우회 bulk write로만 만들어지는 경우

## 5. 대규모 데이터 삽입 후 성능 테스트를 수행하는지 검증

### 검증 목적
- 단순히 대량 데이터를 넣는 것만이 아니라, 삽입 이후 검색 성능 비교 테스트까지 수행되는지 확인합니다.

### 확인 방법
1. 벤치마크 실행

```powershell
make bench-sql-index
```

또는

```powershell
./bench_sql_index
```

2. 다음 항목이 모두 출력되는지 확인합니다.
- `[id index]`
- `[name scan]`
- `[speedup]`

### 충족 판정 기준
- 각 데이터 크기별로 인덱스 검색과 선형 탐색의 시간 비교가 출력되어야 합니다.
- 삽입만 하고 끝나는 것이 아니라, 삽입 후 조회 성능 비교까지 수행되어야 합니다.

### 부분충족 또는 미충족으로 보는 경우
- 대량 삽입은 하지만 성능 비교가 없는 경우
- 성능 비교는 있으나 소규모 데이터만 대상으로 하는 경우

## 6. 데이터 규모 증가에 따른 성능 차이를 분명히 보여주는지 검증

### 검증 목적
- 데이터가 커질수록 `WHERE id = ?` 인덱스 검색과 다른 필드 기준 선형 탐색 사이 차이가 더 분명해지는지 확인합니다.

### 확인 방법
1. 벤치마크 실행

```powershell
make bench-sql-index
```

또는

```powershell
./bench_sql_index
```

2. 세 구간을 비교합니다.
- `rows=10000`
- `rows=100000`
- `rows=1000000`

3. 각 구간의 `speedup` 값을 비교합니다.

### 충족 판정 기준
- 최소한 100만 건까지의 결과가 모두 있어야 합니다.
- 작은 데이터보다 큰 데이터에서 선형 탐색이 더 불리해지는 경향이 보여야 합니다.
- `id index`와 `name scan`의 평균 또는 총 시간 차이가 명확히 드러나야 합니다.

### 부분충족 또는 미충족으로 보는 경우
- 데이터 구간이 너무 작아 차이가 애매한 경우
- 100만 건 결과가 없어 대용량 기준 비교가 완결되지 않는 경우
- 수치가 출력돼도 차이 해석이 어려울 정도로 실험 구성이 약한 경우

## 7. 방금 추가한 INT_MAX 테스트 검증 방법

### 검증 목적
- B+ 트리가 `INT_MAX` 근처의 큰 키 값도 정상적으로 삽입하고 검색하는지 확인합니다.
- 경계값에서 정렬 불변식이나 탐색 로직이 깨지지 않는지 확인합니다.

### 대상 테스트
- `test/test_search.c`의 `test_search_int_max_key()`
- `test/test_structure.c`의 `test_int_max_key_keeps_invariants()`

### 실행 방법
1. 검색 테스트 실행

```powershell
make test_search
```

실행이 생략되면 아래처럼 직접 실행합니다.

```powershell
./test_search
```

2. 구조 테스트 실행

```powershell
make test_structure
```

실행이 생략되면 아래처럼 직접 실행합니다.

```powershell
./test_structure
```

3. 한 번에 실행하려면

```powershell
make test
```

### 충족 판정 기준
- `test_search`에서 `INT_MAX - 1`, `INT_MAX` 삽입 후 검색이 모두 성공해야 합니다.
- `test_structure`에서 큰 키 삽입 후에도 트리 불변식 검사가 통과해야 합니다.
- 프로그램이 assertion 실패 없이 끝까지 완료되어야 합니다.

### 현재 기준 해석
- `test_search` 12/12 PASS이면 search 경계값 검증이 통과한 것입니다.
- `test_structure` 7/7 PASS이면 구조 불변식 경계값 검증이 통과한 것입니다.

### 확실해지는 것
- `INT_MAX` 경계값에서 검색 실패가 없는지
- `INT_MAX` 근처 삽입이 내부 노드/리프 정렬 규칙을 깨지 않는지

### 아직 별도 검증이 필요한 것
- `INT_MIN` 또는 음수 키까지 포함한 전체 정수 범위 테스트
- 매우 큰 데이터셋과 `INT_MAX` 경계값을 동시에 섞는 스트레스 테스트

## 권장 검증 순서

```powershell
./test_storage_insert
./test_search
./test_structure
./bench_sql_index
```

## 최종 판정 기준 요약
- 1번, 3번: `test_storage_insert` 통과 시 storage 레벨에서는 충족으로 볼 수 있습니다.
- 2번: `test_storage_insert` 통과만으로는 부족하고, parser가 일반 VALUES 형태까지 지원하는지 별도 확인해야 합니다.
- 4번, 5번, 6번: `bench_sql_index`가 `rows=1000000`까지 완료되어야 확정할 수 있습니다.
- INT_MAX 테스트: `test_search`, `test_structure` 통과 시 해당 경계값 검증은 확정할 수 있습니다.

즉, 현재 시점에서 문서상 가장 크게 수정되어야 했던 부분은 2번입니다. 이 항목은 storage 테스트 통과와 SQL parser end-to-end 충족을 같은 수준으로 보면 안 되며, 현재 코드는 그 둘을 분리해서 판정하는 것이 맞습니다.
