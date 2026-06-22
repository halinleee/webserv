---
name: webserv-review
description: >
  webserv C++ 코드 리뷰 스킬. C++98 표준 준수, 메모리 누수, FD 관리, HTTP/1.1 정확성, 보안 취약점을 검토한다.
  "리뷰해줘", "검토해줘", "코드 확인", "버그 있어", "버그 찾아줘", "버그 확인", "오류 확인",
  "메모리 누수 확인", "FD 누수", "42 기준 검토", "크래시 원인", "왜 죽어", "동작 안 해",
  "이상한 거 있어", "잘못된 거 있어", "변경사항 리뷰", "diff 확인", "전체 리뷰" 요청 시 이 스킬을 사용할 것.
---

# webserv 코드 리뷰 스킬

## 환경

- **경로**: `/home/seungsch/Webserve`
- **빌드 확인**: `make -C /home/seungsch/Webserve 2>&1`
- **RetStatus**: `STATUS_ERROR=0 / STATUS_OK=1 / STATUS_RE=2`

## 모드 선택

| 요청 패턴 | 모드 | 첫 번째 행동 |
|----------|------|------------|
| "변경사항", "diff", PR 번호 | Quick | `git -C /home/seungsch/Webserve diff main HEAD` |
| 모듈명, "전체" | Full | 관련 `.cpp`/`.hpp` Read |
| "버그", "크래시", "왜", "죽어", "안 돼" | Bug Hunt | 증상 파일부터 역추적 |

## 리뷰 프로세스

1. 모드 결정 → 대상 파일 특정
2. 파일 Read (Quick은 diff, Full/Bug Hunt는 소스 직접)
3. 아래 체크리스트를 P1→P3 순서로 검토
4. `_workspace/review_{module}.md`에 결과 저장
5. implementer 또는 사용자에게 결과 전달

## 체크리스트 (P1 = 즉시 수정 / P2 = 권고 / P3 = 제안)

### P1 — ClientVec 경계 안전성
```
□ client[fd] 접근 전: fd >= 0 && fd < size && client[fd] != NULL
□ pipeToClientMap에서 얻은 clientFd도 동일 범위 검사
□ deleteClient 후 client[fd] = NULL 초기화
```

### P1 — Pipe FD 생명주기
```
□ fork 실패 → inPipe+outPipe 4개 FD 전부 close
□ 자식: dup2 실패 → exit 전 파이프 close
□ 부모: inPipe[1] 쓰기 완료 → epoll DEL → close (CGI stdin EOF 보장)
□ cgiPipeRead 완료 → outPipe[0] epoll DEL → close
□ pipeToClientMap 제거 시 epoll DEL → close 순서
```

### P1 — epoll 이벤트 관리
```
□ EPOLLERR | EPOLLHUP 두 플래그 모두 처리 (비트 OR)
□ FD close 전 epoll_ctl(DEL) 선행
□ 이벤트 배치 내 이미 삭제된 FD 재참조 여부
□ clientAccept 실패 시 accept() FD 누수
```

### P1 — RetStatus 일관성
```
□ if (!func()) 에서 STATUS_RE(2) 누락 처리 여부
□ STATUS_RE 반환 시 호출측 EPOLLOUT 재등록 여부
□ STATUS_ERROR 경로에서 deleteClient/close 일관성
```

### P1 — CGI fork/exec
```
□ execve 실패 → env 배열 해제 후 exit(-1)
□ 타임아웃 → kill(SIGKILL) 후 waitpid 좀비 회수
□ CGI 경로 하드코딩 여부 (Config에서 읽어야 함)
```

### P2 — 시그널
```
□ SIGPIPE → SIG_IGN 설정 여부
□ SIGCHLD → SIG_IGN 또는 waitpid 루프
```

### P2 — C++98 준수
```
□ auto, nullptr, 람다, range-for, shared_ptr 미사용
□ to_string → ostringstream / stoi → atoi
□ 포인터 멤버 → Rule of Three
```

### P2 — 메모리 관리
```
□ new → 모든 에러 경로 delete
□ delete 후 NULL 초기화
□ mapToEnvp char** → freeSplit 호출 경로 완전성
```

### P3 — HTTP/1.1 정확성
```
□ \r\n 구분자 / \n 단독 방어
□ 헤더 이름 소문자 정규화
□ Content-Length 음수/오버플로우
□ ../ 경로 탐색 방어
```

### P3 — 코드 품질
```
□ std::cout 디버그 출력 잔류
□ Doxygen @brief/@param/@return
□ 함수 80행 초과 분리 권고
□ 매직 넘버 상수화
```

## 결과 포맷

```markdown
## 리뷰 결과: {모듈명}  ({Quick/Full/Bug Hunt} 모드)

### [P1] 즉시 수정 필요
- [CRITICAL] {파일}:{라인} — {문제}: {수정 방법}

### [P2] 수정 권고
- [MAJOR] {파일}:{라인} — {문제}: {제안}

### [P3] 개선 제안
- [MINOR] {파일}:{라인} — {제안}

### 통과 항목
- ...

### 결론: 승인 / 재작업 필요
```
