# Git 뼈대 세팅 프롬프트 (세팅 담당자 1명용)

이 프롬프트를 AI에게 전달하면 프로젝트 초기 구조가 한 번에 생성됩니다.

---

```
당신은 C 프로젝트의 Git 레포지토리 초기 세팅을 도와주는 역할입니다.
모든 파일을 직접 생성하고, 마지막에 Git 초기화까지 완료하는 쉘 스크립트(init.sh)를 만들어주세요.

## 프로젝트 개요
- 프로젝트명: bptree-index
- 언어: C
- 목표: 메모리 기반 B+ Tree 인덱스를 구현하고, 기존 SQL 처리기와 연동
- 팀원: 4명 (jiun, minsuk, seokje, sein)
- 작업 방식: 사이클 기반 스프린트
  - 각자 자기 브랜치에서 구현 → 공유 → 베스트 1벌 선정 → main 반영 → 반복

## 1. 디렉토리 구조

bptree-index/
├── src/
│   ├── bptree.h            # B+ Tree 헤더 (구조체, 함수 선언)
│   ├── bptree.c            # B+ Tree 구현
│   ├── sql_processor.h     # SQL 처리기 헤더 (기존 코드에서 가져옴)
│   ├── sql_processor.c     # SQL 처리기 구현 (기존 코드에서 가져옴)
│   └── main.c              # 진입점
├── test/
│   └── test_bptree.c       # B+ Tree 단위 테스트
├── benchmark/
│   └── bench_search.c      # 100만건 벤치마크
├── Makefile
├── .gitignore
├── README.md
└── WORKFLOW.md

## 2. .gitignore
- C 빌드 산출물: *.o, *.out, 실행파일 (bptree, test_bptree, bench_search)
- 에디터/IDE: .vscode/, .idea/, *.swp, *~
- OS: .DS_Store, Thumbs.db

## 3. Makefile
- `make` : src/*.c → bptree 실행파일 빌드
- `make test` : test/test_bptree.c + src/bptree.c 빌드 및 실행
- `make bench` : benchmark/bench_search.c + src/bptree.c 빌드 및 실행
- `make clean` : 빌드 산출물 제거
- 컴파일러: gcc
- 플래그: -Wall -Wextra -O2

## 4. README.md
아래 구조로 작성. TODO 섹션은 빈 칸으로 남겨둠.

# B+ Tree Index

## 팀원
- jiun, minsuk, seokje, sein

## 빌드 및 실행
(make 명령어 설명)

## 프로젝트 구조
(디렉토리 구조 + 각 파일 역할 한 줄 설명)

## B+ Tree 설명
<!-- TODO: 정렬 Phase에서 작성 -->

## 벤치마크 결과
<!-- TODO: 수렴 Phase에서 작성 -->

## 테스트 결과
<!-- TODO: 수렴 Phase에서 작성 -->

## 5. WORKFLOW.md
팀원 참고용 Git 작업 플로우 문서. 아래 내용을 포함.

### 브랜치 구조
main         ← 팀 확정 코드 (사이클마다 선정된 코드만)
├── jiun     ← jiun 개인 작업
├── minsuk   ← minsuk 개인 작업
├── seokje   ← seokje 개인 작업
└── sein     ← sein 개인 작업

### 사이클 작업 흐름 (명령어 포함)

#### 사이클 시작 시
git checkout main
git pull origin main
git checkout <내이름>
git merge main

#### 작업 중 커밋
git add .
git commit -m "[사이클1] 구조체 정의 + search 구현"

#### 선정 후 main 반영 (선정된 사람만 실행)
git checkout main
git merge <내이름>
git push origin main

#### 수렴 단계 (사이클 종료 후 기능 분담)
git checkout main
git pull origin main
git checkout -b feat/benchmark
# 작업 후
git add .
git commit -m "[벤치마크] 100만건 INSERT 측정"
git checkout main
git merge feat/benchmark
git push origin main

### 커밋 메시지 규칙
- [사이클1] : 구조체 + search
- [사이클2] : insert + split
- [사이클3] : SQL 처리기 연동
- [벤치마크] : 성능 측정
- [테스트] : 테스트 코드
- [문서] : README, 주석 등

### 주의사항
- main에 직접 push 금지. 반드시 공유 시간에 합의 후 선정된 사람만 머지.
- 자기 브랜치에서는 자유롭게 커밋해도 OK.

## 6. 소스 파일 초기 내용

### src/bptree.h
- 헤더 가드
- TODO 주석: "정렬 Phase에서 구조체와 함수 선언 추가"

### src/bptree.c
- #include "bptree.h"
- TODO 주석: "사이클 1~2에서 구현"

### src/sql_processor.h
- 헤더 가드
- TODO 주석: "기존 SQL 처리기 코드를 여기에 복사"

### src/sql_processor.c
- #include "sql_processor.h"
- TODO 주석: "기존 SQL 처리기 코드를 여기에 복사"

### src/main.c
- 기본 main 함수
- printf("B+ Tree Index Project\n"); return 0;

### test/test_bptree.c
- #include <assert.h>
- #include "../src/bptree.h"
- void test_create(), test_insert(), test_search(), test_split() 빈 함수 틀
- main에서 각 테스트 호출 + "All tests passed!" 출력

### benchmark/bench_search.c
- #include <time.h>
- #include "../src/bptree.h"
- benchmark_bptree_search(), benchmark_linear_search() 빈 함수 틀
- main에서 각 벤치마크 호출 + 소요시간 출력

## 7. init.sh (쉘 스크립트)
위 파일들을 모두 생성한 뒤 아래 Git 명령어를 실행하는 스크립트.

#!/bin/bash
set -e

# 파일 생성 (위 내용대로)
# ...

# Git 초기화
git init
git add .
git commit -m "init: 프로젝트 초기 구조 세팅"

# 팀원 브랜치 생성
git branch jiun
git branch minsuk
git branch seokje
git branch sein

echo ""
echo "=== 초기 세팅 완료 ==="
echo "다음 단계:"
echo "1. GitHub에 빈 레포 생성"
echo "2. git remote add origin <GITHUB_REPO_URL>"
echo "3. git push -u origin main"
echo "4. git push origin jiun minsuk seokje sein"
echo "5. 팀원에게 clone URL 공유"

## 출력 형식
- 각 파일의 전체 내용을 파일 경로와 함께 출력
- init.sh는 실행 가능한 쉘 스크립트로 작성 (chmod +x 포함)
- 모든 주석과 문서는 한국어로 작성
```

---

## 세팅 담당자 실행 순서

1. 위 프롬프트를 AI에게 전달
2. 생성된 파일들을 프로젝트 폴더에 저장
3. `chmod +x init.sh && ./init.sh` 실행
4. GitHub에 빈 레포 생성
5. `git remote add origin <URL>` → `git push -u origin main`
6. `git push origin jiun minsuk seokje sein`
7. 팀원에게 clone URL + WORKFLOW.md 공유
