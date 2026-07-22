## 리뷰 결과: CGI (Bug Hunt 모드 — 실서버 기동 + 실제 요청 테스트, 2026-06-30 갱신)

### [P1] 좀비 프로세스 누수 — 수정 완료 및 재검증됨

- ~~[CRITICAL]~~ src/server/Server.cpp:395-408 (`cgiPipeRead`) + Server.cpp:471-491 (`deleteClient`) — **CGI 출력이 `MAX_CLIENT_BODY_LENGTH`(1MB) 캡을 넘기거나 `read()`가 에러를 내면(`readCgiPipe`의 RET_ERROR 경로) 자식 프로세스가 영구 좀비로 남음.**
  - **수정 완료**: `Server::reapCgiChild()`(Server.hpp/Server.cpp 신규) 추가 — `deleteClient`와 동일한 waitpid(WNOHANG)→실패 시 kill+blocking waitpid 패턴이지만 `getRunCgi()` 게이트 없이 무조건 시도함. `cgiEventLoop`의 두 에러 분기(cgiPipeRead/cgiPipeWrite 실패 시)에서 `setRunCgi(false)` 호출 직전에 삽입.
  - 1차 수정 시도(매 read마다 `checkCgiExited()` 호출)는 실측으로 불충분함을 확인함 — 캡이 트립되는 시점과 자식이 실제 종료되는 시점이 거의 항상 동시에 일어나서 waitpid가 따라잡을 타이밍이 없었음. `reapCgiChild`로 교체 후 재검증: `cgi_large_output.py` 반복 요청, 동시 요청 10개, crash 케이스 모두 좀비 없음 + FD 카운트 baseline(6) 복귀 확인.
  - 실측: `cgi_large_output.py`(~1.002MB, 캡보다 살짝 큼)를 한 번 요청한 뒤 `ps aux`로 확인하니 `[python3] <defunct>`가 서버 프로세스를 죽일 때까지 영구적으로 남음. 이후 다른 요청을 아무리 보내도 회수되지 않음.
  - 원인: `readCgiPipe()`(Client.cpp:45-64)가 RET_ERROR를 반환하는 경우(read<0, 또는 response.size() > CAP) `checkCgiExited()`/`waitpid()`를 전혀 호출하지 않음 — `checkCgiExited()`는 오직 `length == 0`(EOF) 분기에서만 호출됨. 그리고 `cgiEventLoop`(Server.cpp:134-139)는 `cgiPipeRead` 실패 시 곧바로 `pipeClient->setRunCgi(false)`를 호출함. 클라이언트가 나중에 `deleteClient()`로 정리될 때 fallback reap이 있긴 한데(`if (client->getRunCgi() && waitpid(pid, NULL, WNOHANG) == 0)`, Server.cpp:477), 좌변 `getRunCgi()`가 이미 false라서 **`&&` 단락 평가로 우변 `waitpid()` 호출 자체가 실행되지 않음**. 즉 이 경로로 들어간 자식은 어떤 코드 경로에서도 절대 reap되지 않음.
  - 영향: 거대/비정상 출력 CGI가 반복 호출될 때마다 좀비가 누적 — 프로세스 테이블 고갈로 이어질 수 있는 실질적 DoS.
  - `cgiPipeWrite`(Server.cpp:421-440)의 RET_ERROR 경로도 코드 구조가 완전히 동일해서 같은 버그를 안고 있음(write 실패는 아직 직접 트리거는 안 해봤지만 패턴이 동일).
  - 수정 방향(택1): (a) `readCgiPipe()`/`writeCgiPipe()`의 RET_ERROR 분기 진입 직전 `waitpid(this->pid, NULL, WNOHANG)`로 일단 회수 시도. (b) `cgiPipeRead`/`cgiPipeWrite`의 RET_ERROR 분기에서 `setRunCgi(false)`를 호출하기 전에 `waitpid(client->getPid(), NULL, WNOHANG)`를 먼저 시도해 deleteClient의 fallback이 죽지 않게 함.
  - 참고: [[cgi_exit_code_policy]]에서 합의한 "exit code≠0 → 무조건 RET_ERROR" 정책과는 무관함. 그쪽은 waitpid가 정상 호출되는 경로(EOF 이후 checkCgiExited)의 정책 얘기였고, 이번 건은 waitpid 호출 자체가 누락되는 별개 버그.

### 범위 밖 — 참고용 (다른 팀원의 response 빌더 작업과 겹침, 수정 보류)

- src/server/Server.cpp:159-192 (`clientResponse`) — `Connection: close` 요청 헤더를 전혀 안 읽음. 실측: `Connection: close`를 보낸 요청도 CGI 응답(11ms)이 도착한 뒤 서버가 소켓을 안 닫고 `EPOLLIN` 재등록 + `keepAliveTimeout`(60s) 재설정함. Content-Length가 항상 정확하면 잘 만들어진 클라이언트는 문제없지만 HTTP/1.1 스펙 위반이고, `Connection: close` 클라이언트를 다수 동시에 받으면 연결 슬롯이 불필요하게 점유됨.
- CGI 응답/헤더 파싱 로직, status-line 합성, CGI 에러 시 `client->response.clear()` 누락 — 이전 라운드에서 보고했고, 다른 팀원이 구현 중이라 이번 라운드도 그대로 보류.

### [P2] 수정 권고 (현황 갱신)

- ~~CGI stdout 크기 제한 없음~~ → **해결됨.** `readCgiPipe()`에 `|| this->response.size() > MAX_CLIENT_BODY_LENGTH` 캡 추가됨(Client.cpp:51). 단, 캡이 트립되는 순간 위 신규 P1(좀비 누수)이 같이 발생하니 묶어서 봐야 함.
- ~~`checkCgiExited` exit-code 정책~~ → **결정 완료, 재논의 불필요.** exit code≠0은 절대 기준으로 RET_ERROR 처리하기로 확정([[cgi_exit_code_policy]]).
- [MINOR] src/cgi/Cgi.cpp (`envAppend`) — SCRIPT_NAME/PATH_INFO 미구분, 보고 완료, 아직 미수정. 실측으로 확인: `cgi_env_dump.py` 응답에서 `SCRIPT_NAME=/cgi_bin`로 나옴(스크립트 파일명 `cgi_env_dump.py`가 빠짐) — location prefix만 들어가고 있음을 라이브로 재확인.

### [P3] 개선 제안 (변동 없음)

- [MINOR] include/Client.hpp:165, src/server/Client.cpp:66-71 — `Client::CgiExited()`는 선언/정의만 있고 호출하는 곳이 없음 (`deleteClient`가 동일 로직을 인라인으로 중복 구현).
  - ponytail: L66: delete `Client::CgiExited()` 죽은 코드. `Server::deleteClient`의 인라인 kill/waitpid로 충분.
- [MINOR] src/server/Server.cpp:397,423 / Client.cpp 다수 — CGI 경로에 `std::cout` 디버그 출력이 다수 잔류.

### 통과 항목 (실측 재확인)

- fork 실패/dup2 실패 시 파이프 정리(Cgi.cpp:30-35, Pipe.cpp closeSafely) — 정상.
- CGI 크래시(SIGSEGV) 시 zombie 회수 — 실측 확인: `_tmp_crash.py`(부분 출력 후 self-SIGSEGV)에서 새 좀비 생성 안 됨, checkCgiExited의 WIFSIGNALED 분기가 정상 reap.
- CGI 타임아웃(cgiTimeout=5s) — 실측 확인: 5초 sleep CGI 요청 시 5.04초에 정확히 강제 종료 + 연결 종료, 좀비 생성 안 됨.
- 존재하지 않는 CGI 스크립트(execve 실패) — 실측 확인: 자식이 `exit(-1)`로 깔끔히 종료, 정상 reap, 좀비 없음.
- 동시 요청 10개(서로 다른 QUERY_STRING) — 실측 확인: cross-talk 없이 각자 올바른 응답, FD/좀비 누수 없음.
- 순차 요청 20회 후 서버 FD 개수 — 실측 확인: baseline(6개: stdin/out/err+epoll+listen×2)으로 정확히 복귀, 누수 없음.
- pipeToClientMap erase 순서 및 FD 재사용 안전성(Pipe::closeSafely가 -1로 세팅) — 정상.
- CGI 죽음 직후 클라이언트가 영원히 고아화되는 문제는 이전 라운드에서 수정 완료(cgiEventLoop의 EPOLL_CTL_ADD 전환), 이번 라운드에서도 재발 안 함.

### 결론: 승인 (이번 라운드 신규 P1 1건 수정 및 재검증 완료, 잔여는 P2/P3 + 범위 밖 항목만 남음)
