## 리뷰 결과: 전체 모듈 (Bug Hunt 모드)

### [P1] 즉시 수정 필요

**[CRITICAL] Client.cpp:165 — checkAlive return문 없음 → UB, 타임아웃 시스템 완전 불능**
```cpp
// 현재
bool Client::checkAlive(void) {this->getSocket().checkTimeOut();}
// checkTimeOut() 결과를 반환하지 않음 → C++ UB
// 최적화 시 컴파일러가 항상 0 반환으로 최적화 가능 → 타임아웃 영원히 안 됨

// 수정
bool Client::checkAlive(void) { return (this->getSocket().checkTimeOut()); }
```

**[CRITICAL] Server.cpp:62 — pipeToClientMap 조회 결과에 NULL 체크 없음**
```cpp
Client *pipeClient = this->client[this->pipeToClientMap[currentFd]];
// pipeToClientMap[currentFd] 로 얻은 clientFd 가 이미 deleteClient 된 경우
// this->client[clientFd] == NULL → 66번 줄 pipeClient->getSocket() NULL 역참조
```
수정: `clientExist(this->pipeToClientMap[currentFd])` 체크 후 접근

**[CRITICAL] Server.cpp:286-293 — cgiRun fork 실패 시 pipeToClientMap 잔존 엔트리**
```cpp
this->pipeToClientMap[pipeInFd[1]] = eventSocket;   // 선삽입
this->pipeToClientMap[pipeOutFd[0]] = eventSocket;  // 선삽입
...
if (cgi.excute(...) < 0) {return (STATUS_ERROR);}
// excute 내부에서 fork 실패 시 파이프 4개 모두 close됨
// 하지만 map 엔트리는 그대로! → 해당 fd 번호가 재사용되면 새 클라이언트가 CGI 경로로 오라우팅
```
수정: `excute` 실패 시 map.erase(pipeInFd[1]), map.erase(pipeOutFd[0]) 추가

**[CRITICAL] Server.cpp:230-231 — recv EAGAIN 미처리 → 정상 소켓을 에러 처리**
```cpp
if (length < 0)
    return (STATUS_ERROR);  // EAGAIN/EWOULDBLOCK도 에러 처리
```
논블로킹 소켓에서 데이터 없으면 errno=EAGAIN 으로 -1 반환. 에러가 아님.
수정:
```cpp
if (length < 0)
{
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return (STATUS_OK);
    return (STATUS_ERROR);
}
```

**[CRITICAL] Client.cpp:36 — writeCgiPipe EAGAIN 미처리 → CGI stdin 쓰기 실패 판정**
```cpp
if (written <= 0)
    return (STATUS_ERROR);  // write가 -1+EAGAIN이면 재시도해야 하는데 에러 반환
```
수정:
```cpp
if (written < 0)
{
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return (STATUS_RE);
    return (STATUS_ERROR);
}
if (written == 0)
    return (STATUS_ERROR);
```

---

### [P2] 수정 권고

**[MAJOR] main.cpp — SIGPIPE 미처리 → send/write 실패 시 서버 프로세스 종료**
```cpp
signal(SIGINT, sigIntHandler);
// SIGPIPE 없음 → 클라이언트가 연결 끊은 후 send() 하면 SIGPIPE로 서버 종료
```
수정: `signal(SIGPIPE, SIG_IGN);` 추가

**[MAJOR] Client.cpp:159 — setRunCgi toggle → keep-alive 재요청 시 CGI 실행 안 됨**
```cpp
void Client::setRunCgi(void) {this->runCgi ? this->runCgi = false : this->runCgi = true;}
// CGI 완료 후 runCgi가 true로 남아있음
// 재요청 시 checkRunCgi() == true → cgiRun 스킵
```
수정: `setRunCgi(bool value)` 명시적 setter로 변경

**[MAJOR] Server.cpp:212-218 — epoll ADD 실패 시 deleteClient 없음 → 좀비 클라이언트**
```cpp
if (!(epoll.epollControl(EPOLL_CTL_ADD, tmpFd, EPOLLIN)))
{
    serverSend(this->client[tmpFd]);  // response 비어있어 아무것도 안 보냄
    // deleteClient 없음 → client[tmpFd]와 inClientVec에 남아있음
    // epoll에는 없어서 이벤트 못 받음 → 타임아웃까지 방치
}
```
수정: `deleteClient(tmpFd)` 추가

**[MAJOR] main.cpp:35 — epoll fd 이중 close**
```cpp
server.eventProcess(epoll);
close(epoll.getEpollFd());  // 명시적 close
// ~Epoll() 소멸자에서 또 close → double close
```
수정: `close(epoll.getEpollFd())` 제거

---

### [P3] 개선 제안

- **Cgi.cpp:36** — CGI 경로 하드코딩 (`/home/seungsch/...`). Config 파일에서 읽도록 변경 필요
- **Server.cpp 전체** — `std::cout` 디버그 출력 다수 잔류 (평가 전 제거 권고)
- **Epoll.cpp:63** — `epoll_wait` timeout=0 → 아이들 시 CPU 100%. 100ms 권고

---

### 통과 항목
- runCgi 초기화 (false)
- WIFEXITED 조건 수정
- deleteClient NULL 체크 순서
- inClientVec.erase std::find 수정
- serverSend partial send 수정
- cgiRun pipeInFd[0,1] 누수 수정
- nonblockingSet 실패 시 fd close
- keep-alive DEL→MOD 수정
- dupSetting 실패 exit(-1)
- execve 실패 exit(-1)
- readCgiPipe EAGAIN 처리
- checkCgiExited result==0 분기 추가
- SIGINT epoll_wait 반환 처리

### 결론: 재작업 필요 (P1 5건)
