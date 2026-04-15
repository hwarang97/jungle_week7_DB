# B+ Tree 작업 흐름

## 브랜치 구조

```text
main         ← 팀 확정 코드 (사이클마다 선정된 코드만)
├── jiun     ← jiun 개인 작업
├── minsuk   ← minsuk 개인 작업
├── seokje   ← seokje 개인 작업
└── sein     ← sein 개인 작업
```

## 사이클 작업 흐름

### 사이클 시작 시

```bash
git checkout main
git pull origin main
git checkout <내이름>
git merge main
```

### 작업 중 커밋

```bash
git add .
git commit -m "[사이클1] 구조체 정의 + search 구현"
```

### 선정 후 main 반영

```bash
git checkout main
git merge <내이름>
git push origin main
```

## 수렴 단계

```bash
git checkout main
git pull origin main
git checkout -b feat/benchmark
git add .
git commit -m "[벤치마크] 100만건 INSERT 측정"
git checkout main
git merge feat/benchmark
git push origin main
```

## 커밋 메시지 규칙

- `[사이클1]` : 구조체 + search
- `[사이클2]` : insert + split
- `[사이클3]` : SQL 처리기 연동
- `[벤치마크]` : 성능 측정
- `[테스트]` : 테스트 코드
- `[문서]` : README, 주석 등

## 주의사항

- `main` 에 직접 push 하지 않습니다. 공유 시간에 합의 후 선정된 사람만 머지합니다.
- 개인 브랜치에서는 자유롭게 커밋해도 됩니다.
