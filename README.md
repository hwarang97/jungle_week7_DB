# B+ Tree Index

정글 7주차 DB 프로젝트 레포지토리입니다.
이 저장소는 week6 SQL 처리기 작업물을 기반으로 시작했고, week7에서는 메모리 기반 `B+ Tree` 인덱스를 붙여 `INSERT` / `SELECT` 경로를 개선하는 것을 목표로 합니다.

## 팀원

- jiun
- minsuk
- seokje
- sein

## 프로젝트 목표

- `B+ Tree` 핵심 개념을 팀 전체가 설명할 수 있게 정리
- `search`, `insert`, `split` 의사코드와 한글 주석 기준 확정
- 사이클 방식으로 `구조체 + search`, `insert + split`, `SQL 연동` 구현
- 수렴 단계에서 `벤치마크`, `테스트`, `엣지 케이스`, `코드 정리` 마무리

## 현재 레포 상태

- 기존 week6 SQL 처리기 코드는 그대로 보존되어 있습니다.
- week7 시작을 위해 `B+ Tree` 골격 파일, 테스트 스켈레톤, 벤치마크 스켈레톤, 협업 문서를 추가했습니다.
- 기존 SQL 처리기 실행 경로도 계속 재활용할 수 있도록 `Makefile`에 별도 타깃을 유지했습니다.

## 빌드 및 실행

```bash
make                # B+ Tree 시작용 바이너리 빌드
./bptree

make test           # B+ Tree 스켈레톤 테스트 실행
make bench          # B+ Tree 벤치마크 스켈레톤 실행

make sqlparser      # 기존 week6 SQL 처리기 빌드
make test-sqlparser # 기존 week6 테스트 실행
```

## 프로젝트 구조

### week7 핵심 파일

- `src/bptree.h`: B+ Tree 구조체와 함수 선언
- `src/bptree.c`: B+ Tree 기본 구현 골격
- `src/bptree_main.c`: week7용 시작 진입점
- `src/sql_processor.h`: 기존 SQL 처리기 연결용 헤더
- `src/sql_processor.c`: 기존 SQL 처리기 연결 지점 메모용 구현 파일
- `test/test_bptree.c`: B+ Tree 단위 테스트 스켈레톤
- `benchmark/bench_search.c`: 검색 벤치마크 스켈레톤
- `WORKFLOW.md`: 팀 협업 규칙

### 현재 재활용 중인 기존 week6 파일

- `src/parser.c`
- `src/executor.c`
- `src/storage.c`
- `src/main.c`
- `include/types.h`
- `tests/`

## 오늘 진행 순서

### 정렬 Phase (`11:00~12:00`)

- `B+ Tree` 개념 정리
- `search / insert / split` 의사코드 작성
- 구현용 한글 주석 방향 합의

### 점심 (`12:00~13:00`)

- SQL 처리기와 `B+ Tree`를 어디서 연결할지 가볍게 논의

### 사이클 1 (`13:00~14:00`)

- 각자 `구조체 + search` 구현
- 공유 후 1벌 선정, `main` 반영

### 사이클 2 (`14:00~15:00`)

- 각자 `insert + split` 구현
- 공유 후 1벌 선정, `main` 반영

### 사이클 3 (`15:00~16:00`)

- 각자 SQL `INSERT / SELECT` 연동
- 공유 후 1벌 선정, `main` 반영

### 수렴 단계 (`16:00~19:30`)

- `feat/benchmark`
- `feat/test`
- `feat/edge-case`
- `feat/cleanup`

## 꼭 이해해야 하는 개념

1. 내부 노드와 리프 노드의 역할 차이
2. `search`가 루트에서 리프까지 내려가는 방식
3. `insert` 후 overflow가 나면 `split`이 일어나는 이유
4. 리프 노드 연결이 범위 검색에 주는 이점
5. `INSERT` / `SELECT WHERE id = ...` 에서 인덱스가 연결되는 지점

## 브랜치 전략

- `main`에는 팀이 그 사이클에서 확정한 코드만 올립니다.
- 개인 탐색 브랜치는 `feat/<작업내용>-<이름>` 규칙을 사용합니다.
- 예시: `feat/cycle1-struct-search-jiun`
- 수렴 단계에서는 `feat/benchmark`, `feat/test`, `feat/edge-case`, `feat/cleanup` 같은 기능 브랜치로 전환합니다.

사이클 시작 명령:

```bash
git checkout main
git pull origin main
git checkout <내 브랜치>
git merge main
```

선정 후 `main` 반영:

```bash
git checkout main
git merge <선정된 브랜치>
git push origin main
```

## 참고 문서

- [시간표](docs/timetable.txt)
- [세팅 담당자 문서](docs/prompt-setup-leader.md)
- [팀원 참여 문서](docs/prompt-setup-member.md)
- [B+ Tree 퀴즈](docs/quiz-bptree.md)
- [B+ Tree 퀴즈 답안](docs/quiz-bptree-answer.md)

## B+ Tree 설명

<!-- TODO: 정렬 Phase에서 팀 합의 내용으로 채우기 -->

## 벤치마크 결과

<!-- TODO: 수렴 Phase에서 채우기 -->

## 테스트 결과

<!-- TODO: 수렴 Phase에서 채우기 -->
