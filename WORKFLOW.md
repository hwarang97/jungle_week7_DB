# WORKFLOW

팀 협업은 `main` 고정본 + 개인 탐색 브랜치 + 수렴 브랜치 방식으로 진행합니다.

## 브랜치 구조

```text
main
├── feat/cycle1-struct-search-<name>
├── feat/cycle2-insert-split-<name>
├── feat/cycle3-sql-integration-<name>
├── feat/benchmark
├── feat/test
├── feat/edge-case
└── feat/cleanup
```

## 원칙

- `main`에는 팀에서 선정한 코드만 올립니다.
- 탐색 시간에는 각자 브랜치에서 자유롭게 구현합니다.
- 공유 시간에 코드 비교 후 1벌만 `main`에 반영합니다.
- 선정되지 않은 브랜치는 다음 사이클이 시작되면 버려도 됩니다.

## 사이클 시작

```bash
git checkout main
git pull origin main
git checkout <내 브랜치>
git merge main
```

## 작업 중 커밋

```bash
git add .
git commit -m "[사이클1] 구조체 정의 + search 구현"
```

## 선정 후 main 반영

```bash
git checkout main
git merge <선정된 브랜치>
git push origin main
```

## 선정되지 않았을 때

- 별도 정리는 필수가 아닙니다.
- 다음 사이클 시작 시 `main`을 다시 merge 해서 새로 출발하면 됩니다.

## 수렴 단계

```bash
git checkout main
git pull origin main
git checkout -b feat/benchmark
```

작업 후:

```bash
git add .
git commit -m "[벤치마크] 100만건 INSERT 측정"
git checkout main
git merge feat/benchmark
git push origin main
```

## 커밋 메시지 규칙

- `[사이클1] 구조체 + search`
- `[사이클2] insert + split`
- `[사이클3] SQL 처리기 연동`
- `[벤치마크] 성능 측정`
- `[테스트] 테스트 코드`
- `[문서] README, 주석, 문서 정리`

## 주의사항

- `main`에 직접 push하지 않습니다.
- 공유 시간에 전원 합의 후 선정된 코드만 `main`에 병합합니다.
- 브랜치 이름은 `feat/<작업내용>-<이름>` 규칙을 유지합니다.
