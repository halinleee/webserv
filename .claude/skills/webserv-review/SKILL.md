---
name: webserv-review
description: >
  webserv C++ 코드 리뷰 스킬. C++98 표준 준수, 메모리 누수, FD 관리, HTTP/1.1 정확성, 보안 취약점을 검토한다.
  "리뷰해줘", "검토해줘", "코드 확인", "버그 있어?", "메모리 누수 확인", "42 기준 검토" 요청 시 반드시 이 스킬을 사용할 것.
---

# webserv 코드 리뷰 스킬

## 리뷰 프로세스

1. 변경 파일을 읽는다
2. 아래 체크리스트를 항목별로 순서대로 검토한다
3. 발견된 문제를 CRITICAL / MINOR로 분류한다
4. 결과를 `_workspace/review_{module}.md`에 저장한다

## 체크리스트

### 1. C++98 준수
```
□ auto, nullptr, 람다, range-for, shared_ptr 미사용
□ to_string 대신 ostringstream, stoi 대신 atoi/strtol
□ 복사 생성자/대입 연산자 — 멤버에 포인터 있으면 Rule of Three 적용 여부
□ 초기화 목록(initializer list) 사용 여부 (생성자에서 멤버 초기화)
```

### 2. 메모리 및 자원 관리
```
□ new → 소멸자 또는 에러 경로에서 delete
□ pipe(), socket() → 모든 종료 경로에서 close()
□ 이중 해제(double free) 가능성
□ 벡터/맵에서 포인터 삭제 후 dangling pointer 접근 가능성
```

### 3. HTTP 파싱 정확성
```
□ 줄 구분자: \r\n (CR-LF) 처리, \n 단독 처리 방어
□ 헤더 이름 대소문자 비민감(toLower 후 비교)
□ Content-Length 값 검증 (음수, 오버플로우)
□ 경로 정규화: ../ 탐색 공격 방어 여부
□ 메서드 문자열 검증 (허용된 메서드만 처리)
```

### 4. 논블로킹 I/O
```
□ recv/send 반환값 확인 (-1: 에러, 0: 연결 종료, >0: 읽은 바이트)
□ EAGAIN/EWOULDBLOCK 처리 — 에러가 아님
□ 파이프 write에서 EPIPE 처리 (SIGPIPE 무시 또는 처리)
□ epoll_ctl 실패 시 FD 누수 없음
```

### 5. 보안
```
□ 경로 탐색: realpath() 또는 수동 ../ 제거 로직
□ 버퍼 크기: recv 버퍼 크기 제한 (무한 수신 방지)
□ CGI 환경변수: 사용자 입력값 escape 처리
□ 심볼릭 링크: 루트 디렉토리 밖 접근 방지
```

### 6. 코드 품질
```
□ Doxygen 주석: @brief, @param, @return 완성도
□ 함수 길이: 80행 초과 시 분리 권고
□ 에러 반환 일관성: bool(false/true) 또는 int(-1/0/1)
□ 매직 넘버: 상수로 추출되어 있는지
```

## 문제 분류 기준

| 등급 | 설명 | 대응 |
|------|------|------|
| CRITICAL | 빌드 실패, 메모리 누수, FD 누수, HTTP 응답 오류, 보안 취약점 | 반드시 재작업 |
| MINOR | 코드 스타일, 주석 누락, 변수명, 불필요한 복잡도 | 권고 (implementer 판단) |

## 리뷰 결과 포맷

```markdown
## 리뷰 결과: {모듈명}

### 통과 항목
- ...

### 수정 필요 항목
- [CRITICAL] {파일}:{라인} {문제 설명}: {수정 방법}
- [MINOR] {파일}:{라인} {제안}

### 결론: 승인 / 재작업 필요
```
