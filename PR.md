RequestParser/Request/HttpUtils/Client/Server에 걸쳐 요청 파싱 파이프라인을 확장했다.

구현 내용:
- 헤더 섹션 파싱(parseHeaders/parseKeyValue/validateHeaders) 추가
  - CRLFCRLF 탐색(findCRLFCRLF)으로 헤더 섹션 경계를 찾고, 라인 단위로 key/value 분리
  - 헤더 키는 tchar 검증 후 소문자로 정규화, value는 vchar/SP/HTAB 검증 및 트림
  - obs-fold(헤더 줄 앞 공백/탭) 거부, 헤더 라인/섹션 길이 제한(431) 적용
  - Host 헤더 필수 검증 및 host:port 파싱(parseHost)
  - Content-Length/Transfer-Encoding 동시 존재 거부, 중복 값 검증
    (validateContentLength/validateTransferEncoding), chunked만 지원
  - 같은 키의 여러 헤더를 합치는 transferHeaders (Cookie는 "; ", 그 외는 ", " 구분자)
- 바디 파싱 추가
  - parsePlainBody: Content-Length 기반 고정 길이 바디 수신
  - parseChunkedBody: chunk-size/chunk-data/trailer를 상태 기반으로 순차 파싱,
    누적 바디 크기 제한(413) 적용
- 파싱 상태 머신 정리
  - RequestParser::parse()가 STARTLINE → HEADERS → BODY → DONE을 fallthrough 없이 순차 처리
  - 외부에 노출하는 결과를 ParseResult(PARSE_DONE/PARSE_ERROR/PARSE_INCOMPLETE)로 단순화
  - clear()/handleError()로 파서 내부 상태 초기화 경로 정리
- Client/Server 연동
  - Client::onReceive()가 ParseResult를 반환하도록 변경, 파싱 완료 시 Request를 꺼내 저장
  - Server::clientRequest()가 PARSE_INCOMPLETE면 EPOLLIN 유지, 완료/에러 시 EPOLLOUT 전환
  - Server::clientResponse()에서 shouldClose 플래그로 연결 종료 여부 분기(부분 송신 시 STATUS_RE)
- HttpUtils에 findCRLF 오타 수정(findCRLf→findCRLF), isTchar/isVcharSpTab 추가
- tests/request_parser/ 에 RequestParser 단위 테스트 및 Makefile 추가

TODO (코드 내 주석으로 표시됨):
- validateContentLength/MAX_CLIENT_BODY_LENGTH: 현재 하드코딩값(1000000) 사용 중,
  config에서 파싱한 client_max_body_size 값으로 교체 필요
- Server::clientResponse(): 실제 응답 빌드(buildResponse) 미구현, 현재는 고정 HTML 응답을 보냄
- Server::clientResponse(): 응답 송신 후 RequestParser/Request 상태 초기화 로직 미구현
- Client::onReceive(): PARSE_DONE 처리 후 EPOLLIN 복귀 로직 미구현
- RequestParser.hpp: ParseState를 클래스 내부로 옮기는 것 고려 중
