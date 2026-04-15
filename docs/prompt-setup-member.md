# Git 참여 프롬프트 (나머지 팀원용)

세팅 담당자가 레포를 만든 뒤, 나머지 팀원은 이것만 하면 됩니다.

---

## 1단계: 레포 클론

```bash
git clone <세팅 담당자가 공유한 URL>
cd bptree-index
```

## 2단계: 자기 브랜치로 이동

```bash
# 본인 이름에 해당하는 브랜치로 checkout
git checkout <내이름>

# 예시
git checkout jiun
# 또는
git checkout minsuk
# 또는
git checkout seokje
# 또는
git checkout sein
```

## 3단계: 동작 확인

```bash
make
./bptree
# "B+ Tree Index Project" 출력되면 OK
```

## 4단계: WORKFLOW.md 읽기

```bash
cat WORKFLOW.md
```

작업 흐름, 커밋 규칙이 정리되어 있습니다. 사이클 시작 전에 한 번 읽어두세요.

---

## 매 사이클 시작할 때 (외워두세요)

```bash
git checkout main
git pull origin main
git checkout <내이름>
git merge main
```

이 4줄이면 항상 최신 확정 코드에서 시작할 수 있습니다.

## 작업 중 커밋

```bash
git add .
git commit -m "[사이클1] 구조체 정의 + search 구현"
```

## 내 코드가 선정되었을 때 (선정된 사람만)

```bash
git checkout main
git merge <내이름>
git push origin main
```

## 내 코드가 선정 안 되었을 때

아무것도 안 해도 됩니다.
다음 사이클 시작할 때 main pull 받으면 선정된 코드로 자동 갱신됩니다.
