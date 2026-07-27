## 리뷰 결과: 전체 (Server/Client/Cgi/ServerConfig/Pipe/Epoll/main)  (Full 모드)

### [P1] 즉시 수정 필요

- [CRITICAL] Cgi.cpp:40 (`cmd[1] = getRoot()`) — CGI 위치(location)당 항상 config에 적힌 고정 `root` 경로 하나만 실행됨. 클라이언트가 보낸 실제 요청 경로(`/cgi_bin/a.py`, `/cgi_bin/b.py` 등)는 execve에 전달되는 스크립트 선택에 전혀 반영되지 않음. 같은 location 아래 어떤 .py를 요청해도 늘 같은 파일이 실행됨. → `request.path`에서 location prefix를 제거한 나머지를 `root`와 join해서 실제 스크립트 경로를 만들어야 함(PATH_INFO/SCRIPT_NAME 분리 포함).

### [P2] 수정 권고

- [MAJOR] Client.cpp:143-146 (`checkRunCgi`) — `access(config.getRoot().c_str(), X_OK)`로 스크립트 파일의 **실행 권한**을 검사함. 그런데 실제로 execve는 인터프리터(`cgiPath`)만 직접 실행하고 스크립트는 그 인터프리터의 argv로 넘어갈 뿐이라 스크립트 자체엔 실행권한이 필요 없음(읽기권한만 필요). 스크립트가 `+x` 없이 `644`로만 있으면 CGI 자체가 정상인데도 `checkRunCgi`가 false를 반환해서 CGI가 동작 안 함. → 스크립트 경로는 `R_OK`로 검사해야 함.
- [MAJOR] Server.cpp:285 (`this->configs[client->getListenFd()]`) — `std::map::operator[]`는 키가 없으면 빈 `ServerConfig`를 새로 만들어 끼워 넣음. `listenFd`가 어떤 이유로든 셋팅이 안 된 채(-1 등) 들어오면 조용히 빈 설정(locations 없음)으로 동작하고 디버깅하기 어려운 형태로 실패함(현재는 `clientAccept`가 항상 `setListenFd`를 해주므로 실제로 못 만나는 경로지만, 향후 코드 추가 시 재발 위험). → `find()`로 존재 확인 후 없으면 에러 처리하는 방어 코드 권장.
- [MAJOR] main.cpp — `SIGCHLD`에 대한 처리가 없음. 현재는 `waitpid`를 명시적으로 호출하는 지점(`checkCgiExited`, `deleteClient`, `CgiExited`)이 있어 좀비가 영구적으로 남진 않지만, CGI 자식이 종료된 직후 그 사실을 비동기로 알아챌 방법이 없어서 오직 다음 epoll 이벤트(파이프 EOF)에 의존함. 타이밍 이슈가 생기면 좀비가 잠깐 누적될 수 있음. P1은 아니지만 명시적으로 `signal(SIGCHLD, SIG_IGN)`(자동 reap)나 핸들러를 검토 권장.

### [P3] 개선 제안

- [MINOR] Server.cpp:310 — `std::cout << received << std::endl;`. `received`는 `unsigned char[4096]`라 `operator<<`가 `char*`/`const void*` 오버로드 중 `void*`로 빠져서 사실상 포인터 주소만 찍힘(요청 바디를 보려던 디버그 로그였다면 의도대로 동작하지 않음). 디버그용이면 `reinterpret_cast<char*>(received)`로 캐스팅하거나, 더 이상 안 쓰면 삭제.
- [MINOR] Server.hpp:243 (`bool checkRunCgi();`) — `Server::checkRunCgi()`는 선언만 있고 `.cpp`에 정의가 없고 어디서도 호출되지 않음(실제 동작은 `Client::checkRunCgi(LocationConfig)`가 담당). 죽은 선언.
- ponytail: L243(Server.hpp): yagni 쓰지 않는 `Server::checkRunCgi()` 선언 제거. (`Client::checkRunCgi`가 이미 그 역할을 함)
- ponytail: L310(Server.cpp): delete 디버그용 `std::cout << received` 한 줄. 필요하면 `reinterpret_cast<char*>` 캐스팅해서 의도대로 고치거나 삭제.

### 통과 항목

- Pipe.cpp: 생성/해제/`closeChildSide`/`detach` 경로 전부 FD 누수 없이 깔끔함. `O_NONBLOCK`/`FD_CLOEXEC` 설정도 양쪽 다 적용됨.
- Epoll.cpp: `epWait` 타임아웃이 0(항시 busy-poll)에서 20ms로 수정되어 CPU 100% 이슈 해소됨.
- Server::epollGuard / deleteClient: epoll_ctl 실패 시 일관되게 client 정리하는 단일 경로로 묶여 있음.
- ServerConfig::matching: prefix 경계 체크(`/bin` vs `/bin123`)가 정확하게 구현됨.
- Cgi::excute: fork 실패/실행 실패 양쪽 모두 파이프 정리 경로 존재. execve 실패 시 `env` 해제 후 `exit(-1)`.
- main.cpp: SIGPIPE는 SIG_IGN 처리되어 있어 끊긴 파이프에 write 시 프로세스가 죽는 문제는 없음.

### 결론: 재작업 필요 (P1 1건)
