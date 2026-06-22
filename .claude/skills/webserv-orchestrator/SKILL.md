---
name: webserv-orchestrator
description: >
  webserv HTTP 웹서버 개발 워크플로우 오케스트레이터. implementer(구현), reviewer(리뷰), tester(테스트) 에이전트 팀을
  조율하여 기능을 완성한다. "HTTP 파싱 구현", "Config 파싱", "응답 빌더", "정적 파일 서빙", "리다이렉션",
  "에러 페이지", "CGI 연동", "webserv 기능 추가", "다시 구현", "재실행", "업데이트", "수정해줘",
  "이전 결과 개선", "모듈 추가" 요청 시 반드시 이 스킬을 사용할 것.
---

# webserv 오케스트레이터

## 실행 모드: 에이전트 팀 (파이프라인 패턴)

```
[orchestrator]
    ├── Phase 0: 컨텍스트 확인
    ├── Phase 1: 팀 구성 및 태스크 할당
    │   ├── implementer → 기능 구현
    │   ├── reviewer    → 코드 리뷰 (구현 완료 후)
    │   └── tester      → 테스트 검증 (리뷰 승인 후)
    └── Phase 2: 결과 종합 및 보고
```

## Phase 0: 컨텍스트 확인

워크플로우 시작 전 기존 산출물을 확인하여 실행 모드를 결정한다.

```bash
ls /home/seungsch/Webserve/_workspace/ 2>/dev/null
```

- `_workspace/` 없음 → **초기 실행**: Phase 1부터 전체 수행
- `_workspace/` 있고 부분 수정 요청 → **부분 재실행**: 해당 에이전트만 재호출
- `_workspace/` 있고 새 입력 제공 → **새 실행**: `mv _workspace/ _workspace_prev/` 후 초기 실행

## Phase 1: 팀 구성 및 태스크 할당

### 1-1. 작업 디렉토리 준비

```bash
mkdir -p /home/seungsch/Webserve/_workspace
```

### 1-2. 에이전트 팀 구성

`TeamCreate`로 3명 팀을 구성한다:
- `implementer` — 구현 담당 (webserv-implement 스킬 사용)
- `reviewer` — 리뷰 담당 (webserv-review 스킬 사용)
- `tester` — 테스트 담당

### 1-3. 태스크 할당 순서

파이프라인: 구현 → 리뷰 → (재작업 루프) → 테스트

**implementer 태스크**:
- 요구사항에 따라 `src/{module}/`, `include/` 파일 작성
- Makefile에 새 모듈 등록
- `make re` 빌드 성공 확인
- `_workspace/impl_{module}_done.md` 저장 후 reviewer에게 SendMessage

**reviewer 태스크** (implementer 완료 후):
- `webserv-review` 스킬 기준으로 검토
- `_workspace/review_{module}.md` 저장
- CRITICAL 있으면 implementer에게 재작업 요청
- 승인 시 orchestrator에게 보고 및 tester에게 SendMessage

**tester 태스크** (reviewer 승인 후):
- `tests/` 디렉토리에 테스트 스크립트 작성
- 서버 기동 후 실제 HTTP 요청으로 검증
- `_workspace/test_{module}_result.md` 저장
- 버그 발견 시 implementer에게 버그 리포트 SendMessage

### 1-4. 데이터 전달 프로토콜

| 전달 유형 | 방식 |
|----------|------|
| 에이전트 간 조율 | `SendMessage` |
| 태스크 상태 공유 | `TaskCreate` / `TaskUpdate` |
| 중간 산출물 | `_workspace/` 파일 |
| 최종 산출물 | `src/`, `include/`, `tests/` |

## Phase 2: 결과 종합

모든 에이전트 태스크 완료 후:

1. `_workspace/test_{module}_result.md` 읽어 테스트 통과 여부 확인
2. 미통과 항목 있으면 implementer → reviewer → tester 루프 재실행
3. 전체 통과 시 사용자에게 완료 보고

**보고 포맷**:
```
## 완료: {기능명}

### 구현 파일
- include/{File}.hpp
- src/{module}/{File}.cpp

### 테스트 결과
| 케이스 | 결과 |
|--------|------|
| ...    | PASS |

### 다음 단계 (선택)
- ...
```

## 에러 핸들링

| 상황 | 대응 |
|------|------|
| 빌드 실패 | implementer가 즉시 수정, 재빌드 |
| 리뷰 CRITICAL | implementer 재작업 (최대 2회 루프) |
| 테스트 실패 | implementer 버그 수정 후 tester 재실행 |
| 2회 이상 반복 실패 | orchestrator가 사용자에게 에스컬레이션 |

## 테스트 시나리오

### 정상 흐름: HTTP 파싱 모듈 구현
1. orchestrator: `_workspace/` 없음 확인 → 초기 실행
2. implementer: `HttpRequest.hpp`, `src/http/HttpRequest.cpp` 작성 + 빌드
3. reviewer: C++98 준수, 파싱 정확성 검토 → 승인
4. tester: `tests/http_parse_test.sh` 실행 → 전체 PASS
5. orchestrator: 완료 보고

### 에러 흐름: 빌드 실패
1. implementer: 구현 후 `make re` → 컴파일 에러
2. implementer: 에러 수정 후 재빌드 성공
3. (이후 정상 흐름 계속)

## webserv 구현 로드맵 (참고)

| 우선순위 | 모듈 | 파일 |
|---------|------|------|
| 1 | HTTP 요청 파싱 | `src/http/HttpRequest.cpp` |
| 2 | HTTP 응답 빌더 | `src/http/HttpResponse.cpp` |
| 3 | Config 파싱 | `src/config/Config.cpp` |
| 4 | 정적 파일 서빙 | `Server::serveStatic()` |
| 5 | 에러 페이지 커스터마이징 | Config 연동 |
| 6 | 리다이렉션 | 301/302 응답 |
| 7 | 디렉토리 리스팅 | autoindex |
